#include "opt/pass.h"
#include "opt/passman.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  dead code elimination
  this pass will remove unused instructions
*/
class DeadCodeEliminationPass : public BlockPass {
 public:
  DeadCodeEliminationPass() {}

  bool RunOnBlock(const BlockPtr &block) override {
    bool changed = false;
    // traverse all instructions
    auto &insts = block->insts();
    for (auto it = insts.begin(); it != insts.end();) {
      remove_flag_ = false;
      (*it)->RunPass(*this);
      // check if need to be removed
      if (remove_flag_) {
        it = insts.erase(it);
        changed = true;
      }
      else {
        ++it;
      }
    }
    return changed;
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
  bool remove_flag_;
};

}  // namespace

// register current pass
REGISTER_PASS(DeadCodeEliminationPass, dead_code_elim)
    .set_min_opt_level(0)
    .set_stages(PassStage::PreOpt | PassStage::Opt | PassStage::PostOpt);
