#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/helper/cast.h"
#include "opt/analysis/loopinfo.h"
#include "opt/helper/const.h"
#include "mid/module.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

// helper pass for creating declaration of 'memset'
class CreateMemSetPass : public ModulePass {
 public:
  CreateMemSetPass() {}

  bool RunOnModule(UserPtrList &global_vals) override {
    using namespace mimic::define;
    if (global_vals.empty()) return false;
    if (!IsSSA<FunctionSSA>(global_vals.front())) return false;
    // traverse all function declarations
    // find if there is already a definition here
    for (const auto &i : global_vals) {
      auto func = SSACast<FunctionSSA>(i);
      if (func->is_decl() && func->name() == "memset") {
        memset_decl_ = func;
        return false;
      }
    }
    // create new declaration
    auto mod = MakeModule(global_vals.front()->logger());
    auto ptr_ty = MakePointer(MakeVoid(), false);
    auto int_ty = MakePrimType(PrimType::Type::Int32, false);
    TypePtrList args = {ptr_ty, int_ty, int_ty};
    auto func_ty = std::make_shared<FuncType>(std::move(args),
                                              std::move(ptr_ty), false);
    auto decl = mod.CreateFunction(LinkageTypes::External,
                                   "memset", func_ty);
    // insert into global values
    global_vals.push_front(decl);
    // update stored value
    memset_decl_ = SSACast<FunctionSSA>(decl);
    return true;
  }

  // getters
  const FuncPtr &memset_decl() const { return memset_decl_; }

 private:
  FuncPtr memset_decl_;
};

// register helper pass
REGISTER_PASS(CreateMemSetPass, create_memset)
    .set_is_analysis(true);


/*
  convert loop to another equivalent form
  this pass will try to convert loops info 'memset'
*/
class LoopConversionPass : public FunctionPass {
 public:
  LoopConversionPass() {}

  bool RunOnFunction(const FuncPtr &func) override {
    if (func->is_decl()) return false;
    // run on loops
    const auto &li = PassManager::GetPass<LoopInfoPass>("loop_info");
    const auto &loops = li.GetLoopInfo(func.get());
    for (const auto &loop : loops) {
      if (RunOnLoop(loop)) return true;
    }
    return false;
  }

 private:
  bool CheckLoop(const LoopInfo &loop);
  bool RunOnLoop(const LoopInfo &loop);
};

}  // namespace


// register current pass
REGISTER_PASS(LoopConversionPass, loop_conv)
    .set_min_opt_level(2)
    .set_stages(PassStage::Opt)
    .Requires("create_memset")
    .Requires("loop_info")
    .Requires("licm")
    .Invalidates("dom_info");


// check if current loop can be converted to a 'memset' call
bool LoopConversionPass::CheckLoop(const LoopInfo &loop) {
  using Op = BinarySSA::Operator;
  // loop must contain only 2 blocks
  // entry must contain only 3 instructions (ind_var, end_cond, branch)
  if (loop.body.size() != 2 || loop.entry->insts().size() != 3) {
    return false;
  }
  // initial value must be zero
  if (!loop.init_val) return false;
  auto cint = ConstantHelper::Fold(loop.init_val->GetPointer());
  if (!cint || cint->value()) return false;
  // modifier must be an 'add %ind, 1'
  if (!loop.modifier || loop.modifier->op() != Op::Add ||
      loop.modifier->lhs().get() != loop.ind_var) {
    return false;
  }
  cint = ConstantHelper::Fold(loop.modifier->rhs());
  if (!cint || cint->value() != 1) return false;
  // condition must be a 'slt', and it's lhs must be induction variable
  if (!loop.end_cond || loop.end_cond->op() != Op::SLess ||
      loop.end_cond->lhs().get() != loop.ind_var) {
    return false;
  }
  return true;
}

// perform conversion
bool LoopConversionPass::RunOnLoop(const LoopInfo &loop) {
  SSAPtr ptr, zero, count;
  if (!CheckLoop(loop)) return false;
  count = loop.end_cond->rhs();
  // body block must contain only 4 instructions
  // (access, store, modifier, jump)
  assert(loop.body_block);
  if (loop.body_block->insts().size() != 4) return false;
  // traverse all instructions
  std::size_t index = 0;
  AccessSSA *acc;
  for (const auto &i : loop.body_block->insts()) {
    switch (index) {
      case 0: {
        // must be an access instruction
        acc = SSADynCast<AccessSSA>(i.get());
        if (!acc) return false;
        // index must be induction variable
        if (acc->index().get() != loop.ind_var) return false;
        ptr = acc->ptr();
        break;
      }
      case 1: {
        // must be an store instruction
        auto store = SSADynCast<StoreSSA>(i.get());
        if (!store) return false;
        // value must be zero
        auto val = ConstantHelper::Fold(store->value());
        if (!val || val->value()) return false;
        zero = val;
        // pointer must be the last access instruction
        if (store->ptr().get() != acc) return false;
        break;
      }
      case 2: {
        // must be modifier
        if (i.get() != loop.modifier) return false;
        break;
      }
      default:;
    }
    ++index;
  }
  // remove loop body
  loop.body_block->Clear();
  loop.body_block->ClearInst();
  loop.body_block->parent()->RemoveValue(loop.body_block);
  // update predecessors of entry block
  loop.entry->RemoveValue(loop.tail);
  // replace induction variable
  loop.ind_var->ReplaceBy(count);
  // remove all instructions
  loop.entry->insts().clear();
  // insert function call
  auto entry = SSACast<BlockSSA>(loop.entry->GetPointer());
  auto mod = MakeModule(loop.entry->logger(), entry);
  const auto &cm = PassManager::GetPass<CreateMemSetPass>("create_memset");
  const auto &callee = cm.memset_decl();
  SSAPtrList args = {std::move(ptr), std::move(zero), std::move(count)};
  mod.CreateCall(callee, std::move(args));
  // insert jump instruction
  auto jump = std::make_shared<JumpSSA>(loop.exit_block->GetPointer());
  loop.entry->insts().push_back(std::move(jump));
  return true;
}
