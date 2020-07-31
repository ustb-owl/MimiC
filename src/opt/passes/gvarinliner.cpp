#include <unordered_map>
#include <cstddef>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/passes/helper/cast.h"
#include "mid/module.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  this pass will inline global variables
  that has only been used in one function
*/
class GlobalVariableInliner : public FunctionPass {
 public:
  GlobalVariableInliner() {}

  bool RunOnFunction(const UserPtr &func) override {
    // scan for all uses of global variables
    for (const auto &i : *func) {
      auto block = SSACast<BlockSSA>(i.value().get());
      for (const auto &inst : block->insts()) {
        inst->RunPass(*this);
      }
    }
    // find insert point after all allocations
    // for interting initialization instructions
    if (gvar_counter_.empty()) return false;
    SSAPtrList::iterator pos;
    auto entry = SSACast<BlockSSA>((*func)[0].value());
    auto &insts = entry->insts();
    for (auto it = insts.begin(); it != insts.end(); ++it) {
      if (!IsSSA<AllocaSSA>(*it)) {
        pos = it;
        break;
      }
    }
    // handle all target global variables
    bool changed = false;
    for (const auto &[gvar, use_count] : gvar_counter_) {
      if (gvar->uses().size() == use_count) {
        if (!changed) changed = true;
        // create alloca
        auto mod = MakeModule(gvar->logger(), entry, insts.begin());
        auto type = gvar->type()->GetDerefedType();
        auto alloca = mod.CreateAlloca(type);
        // create initializer
        mod.SetInsertPoint(entry, pos);
        if (gvar->init()) {
          mod.CreateStore(gvar->init(), alloca);
        }
        else {
          mod.CreateStore(mod.GetZero(type), alloca);
        }
        // replace all uses with created alloca
        gvar->ReplaceBy(alloca);
      }
    }
    // release resources
    gvar_counter_.clear();
    return changed;
  }

  void RunOn(LoadSSA &ssa) override {
    LogGlobalVar(ssa.ptr());
  }

  void RunOn(StoreSSA &ssa) override {
    LogGlobalVar(ssa.ptr());
  }

  void RunOn(AccessSSA &ssa) override {
    LogGlobalVar(ssa.ptr());
  }

 private:
  void LogGlobalVar(const SSAPtr &val) {
    if (auto gvar = SSADynCast<GlobalVarSSA>(val.get())) {
      gvar_counter_[gvar] += 1;
    }
  }

  // use counter of global variables
  std::unordered_map<GlobalVarSSA *, std::size_t> gvar_counter_;
};

}  // namespace

// register current pass
REGISTER_PASS(GlobalVariableInliner, gvar_inliner, 1, PassStage::PreOpt);
