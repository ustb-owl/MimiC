#include "opt/pass.h"
#include "opt/passman.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

// dead global value elimination
// NOTE: this pass will only remove unused function declarations
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
    // remove unused declarations
    remove_flag_ = !ssa.size() && ssa.uses().empty();
  }

 private:
  // set if need to be removed
  bool remove_flag_;
};

}  // namespace

// register current pass
REGISTER_PASS(DeadGlobalValEliminationPass, dead_glob_elim, 0, false);
