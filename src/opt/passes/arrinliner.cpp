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
  parameters of pass
*/
// size threshold of global array
constexpr std::size_t kThresholdSize = 1024 * 1024 * 4;


/*
  this pass will inline global arrays
  that has only been used in 'main' function
*/
class GlobalArrayInliner : public FunctionPass {
 public:
  GlobalArrayInliner() {}

  bool RunOnFunction(const UserPtr &func) override {
    auto func_ptr = SSACast<FunctionSSA>(func.get());
    if (func_ptr->name() != "main") return false;
    // scan for all uses of global arrays
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
    auto entry = SSACast<BlockSSA>(func_ptr->entry());
    auto &insts = entry->insts();
    for (auto it = insts.begin(); it != insts.end(); ++it) {
      if (!IsSSA<AllocaSSA>(*it)) {
        pos = it;
        break;
      }
    }
    // handle all target global arrays
    bool changed = false;
    for (const auto &[gvar, use_count] : gvar_counter_) {
      auto type = gvar->type()->GetDerefedType();
      if (gvar->uses().size() == use_count &&
          type->GetSize() <= kThresholdSize) {
        if (!changed) changed = true;
        // create alloca
        auto mod = MakeModule(gvar->logger(), entry, insts.begin());
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

  void RunOn(CastSSA &ssa) override {
    LogGlobalVar(ssa.opr());
  }

 private:
  void LogGlobalVar(const SSAPtr &val) {
    if (auto gvar = SSADynCast<GlobalVarSSA>(val.get())) {
      // handle array only
      if (gvar->type()->GetDerefedType()->IsArray()) {
        gvar_counter_[gvar] += 1;
      }
    }
  }

  // use counter of global arrays
  std::unordered_map<GlobalVarSSA *, std::size_t> gvar_counter_;
};

}  // namespace

// register current pass
REGISTER_PASS(GlobalArrayInliner, arr_inliner, 2, PassStage::Opt);
