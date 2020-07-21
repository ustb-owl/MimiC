#include "opt/pass.h"
#include "opt/passman.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  dead code elimination
  this pass will remove unused instructions
*/
class DeadCodeEliminationPass : public FunctionPass {
 public:
  DeadCodeEliminationPass() {}

  bool RunOnFunction(const UserPtr &func) override {
    changed_ = false;
    // traverse all basic blocks
    for (const auto &i : *func) {
      i.value()->RunPass(*this);
    }
    return changed_;
  }

  void RunOn(BlockSSA &ssa) override {
    // traverse all instructions
    for (auto it = ssa.insts().begin(); it != ssa.insts().end();) {
      remove_flag_ = false;
      (*it)->RunPass(*this);
      // check if need to be removed
      if (remove_flag_) {
        it = ssa.insts().erase(it);
        changed_ = true;
      }
      else {
        ++it;
      }
    }
  }

  void RunOn(LoadSSA &ssa) override {
    if (ssa.uses().empty()) remove_flag_ = true;
  }

  void RunOn(AccessSSA &ssa) override {
    if (ssa.uses().empty()) remove_flag_ = true;
  }

  void RunOn(BinarySSA &ssa) override {
    if (ssa.uses().empty()) remove_flag_ = true;
  }

  void RunOn(UnarySSA &ssa) override {
    if (ssa.uses().empty()) remove_flag_ = true;
  }

  void RunOn(CastSSA &ssa) override {
    if (ssa.uses().empty()) remove_flag_ = true;
  }

  void RunOn(AllocaSSA &ssa) override {
    if (ssa.uses().empty()) {
      remove_flag_ = true;
      ssa.logger()->LogWarning("unused variable definition");
    }
  }

  void RunOn(PhiSSA &ssa) override {
    if (ssa.uses().empty()) remove_flag_ = true;
  }

  void RunOn(SelectSSA &ssa) override {
    if (ssa.uses().empty()) remove_flag_ = true;
  }

 private:
  bool changed_, remove_flag_;
};

}  // namespace

// register current pass
<<<<<<< HEAD
REGISTER_PASS(DeadCodeEliminationPass, dead_code_elim, 0,
              PassStage::PreOpt | PassStage::Opt | PassStage::PostOpt);
=======
REGISTER_PASS(DeadCodeEliminationPass, dead_code_elim, 0);
>>>>>>> daacd85... (passman) removed option 'is_analysis'
