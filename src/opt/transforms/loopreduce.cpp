#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/helper/inst.h"
#include "opt/analysis/loopinfo.h"
#include "opt/helper/const.h"
#include "opt/helper/cast.h"
#include "mid/module.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  this pass will perform induction variable strength reduction
  on all access instruction in loop
*/
class LoopStrengthReductionPass : public FunctionPass {
 public:
  LoopStrengthReductionPass() {}

  bool RunOnFunction(const FuncPtr &func) override {
    if (func->is_decl()) return false;
    // initialize parent scanner
    ParentScanner parent(func);
    parent_ = &parent;
    // run on loops
    const auto &li = PassManager::GetPass<LoopInfoPass>("loop_info");
    const auto &loops = li.GetLoopInfo(func.get());
    for (const auto &loop : loops) {
      if (RunOnLoop(loop)) return true;
    }
    return false;
  }

  void RunOn(AccessSSA &ssa) override;

 private:
  bool CheckLoop(const LoopInfo &loop);
  bool RunOnLoop(const LoopInfo &loop);

  ParentScanner *parent_;
  bool changed_;
  const LoopInfo *cur_loop_;
  SSAPtr step_;
};

}  // namespace

// register current pass
REGISTER_PASS(LoopStrengthReductionPass, loop_reduce)
    .set_min_opt_level(2)
    .set_stages(PassStage::Opt)
    .Requires("loop_info")
    .Requires("licm")
    .Requires("loop_conv");


// check if the specific loop can be handled
bool LoopStrengthReductionPass::CheckLoop(const LoopInfo &loop) {
  if (!loop.preheader || !loop.modifier || !loop.ind_var) return false;
  // modifier must be an instruction like 'add %ind, %x'
  if (loop.modifier->op() != BinarySSA::Operator::Add) return false;
  if (loop.modifier->lhs().get() == loop.ind_var) {
    step_ = ConstantHelper::Fold(loop.modifier->rhs());
  }
  else if (loop.modifier->rhs().get() == loop.ind_var) {
    step_ = ConstantHelper::Fold(loop.modifier->lhs());
  }
  else {
    return false;
  }
  // step must be a constant
  if (!step_) return false;
  return true;
}

// handle current loop
bool LoopStrengthReductionPass::RunOnLoop(const LoopInfo &loop) {
  changed_ = false;
  if (!CheckLoop(loop)) return changed_;
  // traverse all users of induction variable
  cur_loop_ = &loop;
  auto uses = loop.ind_var->uses();
  for (const auto &i : uses) {
    auto block = parent_->GetParent(i->user());
    if (loop.body.count(block)) {
      i->user()->RunPass(*this);
    }
  }
  return changed_;
}

void LoopStrengthReductionPass::RunOn(AccessSSA &ssa) {
  using AccTy = AccessSSA::AccessType;
  if (cur_loop_->ind_var != ssa.index().get()) return;
  auto tail = SSACast<BlockSSA>(cur_loop_->tail->GetPointer());
  // create initial value in preheader
  auto phdr = SSACast<BlockSSA>(cur_loop_->preheader->GetPointer());
  auto mod = MakeModule(ssa.logger(), phdr, --phdr->insts().end());
  auto ptr = ssa.ptr();
  if (ssa.acc_type() == AccTy::Element) {
    // create a cast instruction as pointer
    ptr = mod.CreateCast(ssa.ptr(), ssa.type());
  }
  auto init = mod.CreatePtrAccess(ptr, cur_loop_->init_val->GetPointer());
  // create phi node in loop entry
  auto entry = SSACast<BlockSSA>(cur_loop_->entry->GetPointer());
  mod.SetInsertPoint(entry, entry->insts().begin());
  auto phi = mod.CreatePhi({mod.CreatePhiOperand(init, phdr)});
  // create modifier of current access
  mod = MakeModule(ssa.logger(), tail, --tail->insts().end());
  auto acc_mod = mod.CreatePtrAccess(phi, step_);
  phi->AddValue(mod.CreatePhiOperand(acc_mod, tail));
  // replace current access
  ssa.ReplaceBy(phi);
  parent_->GetParent(&ssa)->insts().remove_if(
      [&ssa](const SSAPtr &i) { return i.get() == &ssa; });
  // mark as changed
  changed_ = true;
}
