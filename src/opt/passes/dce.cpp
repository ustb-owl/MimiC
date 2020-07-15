#include <forward_list>

#include "opt/pass.h"
#include "opt/passman.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

// dead code elimination
class DeadCodeEliminationPass : public FunctionPass {
 public:
  DeadCodeEliminationPass() {}

  bool RunOnFunction(const UserPtr &func) override {
    changed_ = false;
    cur_func_ = func.get();
    removed_blocks_.clear();
    // traverse all basic blocks
    for (const auto &i : *func) {
      i.value()->RunPass(*this);
    }
    // remove all marked blocks
    for (const auto &i : removed_blocks_) {
      i->RemoveFromUser();
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
    // remove non-entry blocks with no predecessors
    if (ssa.empty() && (*cur_func_)[0].value().get() != &ssa) {
      if (ssa.insts().size() > 1) {
        ssa.logger()->LogWarning("unreachable code");
      }
      // mark as removed
      removed_blocks_.push_front(&ssa);
      changed_ = true;
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

 private:
  // set if IR changed
  bool changed_;
  // current function
  User *cur_func_;
  // set if instruction need to be removed
  bool remove_flag_;
  // list of blocks that need to be removed
  std::forward_list<BlockSSA *> removed_blocks_;
};

}  // namespace

// register current pass
REGISTER_PASS(DeadCodeEliminationPass, dead_code_elim, 0, false);
