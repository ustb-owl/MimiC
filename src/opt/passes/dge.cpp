#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/helper/cast.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  dead global value elimination
  this pass will:
  1.  remove unused function declarations
  2.  remove unused internal/inline functions and global variables
*/
class DeadGlobalValEliminationPass : public ModulePass {
 public:
  DeadGlobalValEliminationPass() {}

  bool RunOnModule(UserPtrList &global_vals) override {
    bool changed = false;
    // traverse all global values
    for (auto it = global_vals.begin(); it != global_vals.end();) {
      remove_flag_ = false;
      (*it)->RunPass(*this);
      if (remove_flag_) {
        it = global_vals.erase(it);
        if (!changed) changed = true;
      }
      else {
        ++it;
      }
    }
    return changed;
  }

  void RunOn(FunctionSSA &ssa) override {
    if (ssa.uses().empty()) {
      // check if is internal
      bool is_internal = IsInternal(ssa.link());
      if (is_internal) {
        ssa.logger()->LogWarning("unused internal function definition");
      }
      // mark if need to be removed
      remove_flag_ = !ssa.size() || is_internal;
      // release blocks if function will be removed
      // because there may be circular referenced caused by control flow
      if (remove_flag_) {
        for (const auto &block : ssa) {
          auto block_ptr = SSACast<BlockSSA>(block.value().get());
          block_ptr->Clear();
          block_ptr->insts().clear();
        }
        ssa.Clear();
      }
    }
  }

  void RunOn(GlobalVarSSA &ssa) override {
    // check if need to be removed
    bool is_internal = IsInternal(ssa.link());
    if (ssa.uses().empty() && is_internal) {
      ssa.logger()->LogWarning("unused internal global variable");
      remove_flag_ = true;
    }
  }

 private:
  bool IsInternal(LinkageTypes link) {
    return link == LinkageTypes::Internal || link == LinkageTypes::Inline;
  }

  // set if need to be removed
  bool remove_flag_;
};

}  // namespace

// register current pass
REGISTER_PASS(DeadGlobalValEliminationPass, dead_glob_elim)
    .set_min_opt_level(0)
    .set_stages(PassStage::PreOpt | PassStage::Opt);
