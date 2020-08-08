#include <cassert>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/analysis/loopinfo.h"
#include "mid/module.h"
#include "opt/helper/cast.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  this pass will add preheader block before loop when necessary
*/
class LoopNormalizePass : public FunctionPass {
 public:
  LoopNormalizePass() {}

  bool RunOnFunction(const FuncPtr &func) override {
    if (func->is_decl()) return false;
    // get loop info
    const auto &li = PassManager::GetPass<LoopInfoPass>("loop_info");
    const auto &loops = li.GetLoopInfo(func.get());
    // traverse all loops
    bool changed = false;
    for (const auto &loop : loops) {
      if (loop.entry->size() > 2) {
        CreatePreheader(loop);
        changed = true;
      }
    }
    return changed;
  }

  void CleanUp() override {
    preheader_ = nullptr;
  }

  void RunOn(BranchSSA &ssa) override;
  void RunOn(JumpSSA &ssa) override;

 private:
  void CreatePreheader(const LoopInfo &loop);
  void ReroutePhi(const LoopInfo &loop);

  BlockPtr preheader_;
  BlockSSA *loop_entry_;
};

}  // namespace

// register pass
REGISTER_PASS(LoopNormalizePass, loop_norm)
    .set_min_opt_level(2)
    .set_stages(PassStage::Opt)
    .Requires("loop_info")
    .Invalidates("dom_info");


void LoopNormalizePass::CreatePreheader(const LoopInfo &loop) {
  // create preheader block
  auto mod = MakeModule(loop.entry->logger());
  preheader_ = mod.CreateBlock(loop.entry->parent(),
                               loop.entry->name() + ".preheader");
  // set up predecessors
  for (auto &&i : *loop.entry) {
    if (i.value().get() != loop.tail) {
      preheader_->AddValue(i.value());
      i.set_value(nullptr);
    }
  }
  loop.entry->RemoveValue(nullptr);
  loop.entry->AddValue(preheader_);
  // reroute predecessor's successor
  loop_entry_ = loop.entry;
  for (const auto &i : *preheader_) {
    auto pred = SSACast<BlockSSA>(i.value().get());
    pred->insts().back()->RunPass(*this);
  }
  // create jump to loop entry
  assert(!loop.entry->uses().empty());
  const auto &entry_ptr = loop.entry->uses().front()->value();
  auto jump = std::make_shared<JumpSSA>(entry_ptr);
  jump->set_logger(loop.entry->logger());
  preheader_->insts().push_back(std::move(jump));
  // reroute phi nodes in loop header
  ReroutePhi(loop);
}

void LoopNormalizePass::ReroutePhi(const LoopInfo &loop) {
  if (preheader_->size() == 1) {
    // phi only has two operands, update incoming block
    for (const auto &i : loop.entry->insts()) {
      if (auto phi = SSADynCast<PhiSSA>(i.get())) {
        for (const auto &opr : *phi) {
          auto opr_ptr = SSACast<PhiOperandSSA>(opr.value().get());
          if (opr_ptr->block().get() != loop.tail) {
            opr_ptr->set_block(preheader_);
            break;
          }
        }
      }
    }
  }
  else {
    assert(preheader_->size() > 1);
    // phi has more than two operands, place a new phi in preheader
    for (const auto &i : loop.entry->insts()) {
      if (auto phi = SSADynCast<PhiSSA>(i.get())) {
        // extract operands from non-tail block
        SSAPtrList oprs;
        for (auto &&opr : *phi) {
          auto opr_ptr = SSACast<PhiOperandSSA>(opr.value().get());
          if (opr_ptr->block().get() != loop.tail) {
            oprs.push_back(opr.value());
            opr.set_value(nullptr);
          }
        }
        phi->RemoveValue(nullptr);
        // create new phi node
        auto mod = MakeModule(phi->logger());
        auto new_phi = mod.CreatePhi(oprs);
        // add the new phi node to the old one
        phi->AddValue(mod.CreatePhiOperand(new_phi, preheader_));
        // insert the new phi node to preheader
        preheader_->insts().push_front(std::move(new_phi));
      }
    }
  }
}

void LoopNormalizePass::RunOn(BranchSSA &ssa) {
  if (ssa.true_block().get() == loop_entry_) {
    ssa.set_true_block(preheader_);
  }
  else {
    assert(ssa.false_block().get() == loop_entry_);
    ssa.set_false_block(preheader_);
  }
}

void LoopNormalizePass::RunOn(JumpSSA &ssa) {
  assert(ssa.target().get() == loop_entry_);
  ssa.set_target(preheader_);
}
