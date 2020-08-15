#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "opt/pass.h"
#include "opt/passman.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  a naive implementation of local dead store elimination
  this pass will eliminate dead stores

  TODO: alias analysis
*/
class DeadStoreEliminationPass : public BlockPass {
 public:
  DeadStoreEliminationPass() {}

  bool RunOnBlock(const BlockPtr &block) override {
    // traverse all instructions
    for (const auto &i : block->insts()) {
      i->RunPass(*this);
    }
    // remove all dead stores
    for (const auto &store : dead_stores_) {
      block->insts().remove_if(
          [store](const SSAPtr &i) { return i.get() == store; });
    }
    return !dead_stores_.empty();
  }

  void CleanUp() override {
    tracked_ptrs_.clear();
    ptr_defs_.clear();
    dead_stores_.clear();
  }

  void RunOn(AllocaSSA &ssa) override {
    // track alloca anyway
    tracked_ptrs_.insert(&ssa);
  }

  void RunOn(AccessSSA &ssa) override {
    const auto &base = ssa.ptr();
    // try to update tracked pointers
    if (!tracked_ptrs_.count(base.get())) {
      base->RunPass(*this);
    }
    // track access only if it's base pointer has been tracked
    if (tracked_ptrs_.count(base.get())) {
      tracked_ptrs_.insert(&ssa);
    }
  }

  void RunOn(CastSSA &ssa) override {
    const auto &opr = ssa.opr();
    if (!opr->type()->IsPointer()) return;
    // try to update tracked pointers
    if (!tracked_ptrs_.count(opr.get())) {
      opr->RunPass(*this);
    }
    // track cast only if it's operand has been tracked
    if (tracked_ptrs_.count(opr.get())) {
      tracked_ptrs_.insert(&ssa);
    }
  }

  void RunOn(StoreSSA &ssa) override {
    // try to update tracked pointers
    const auto &ptr = ssa.ptr();
    ssa.ptr()->RunPass(*this);
    if (!tracked_ptrs_.count(ptr.get())) return;
    // check if the pointer has already been stored
    auto &def = ptr_defs_[ptr.get()];
    if (def) {
      // dead store found
      dead_stores_.push_back(def);
    }
    // update pointer definitions
    def = &ssa;
  }

  void RunOn(LoadSSA &ssa) override {
    ptr_defs_.clear();
  }

  void RunOn(CallSSA &ssa) override {
    ptr_defs_.clear();
  }

 private:
  // all tracked pointers
  std::unordered_set<Value *> tracked_ptrs_;
  std::unordered_map<Value *, StoreSSA *> ptr_defs_;
  std::vector<StoreSSA *> dead_stores_;
};

}  // namespace


// register pass
REGISTER_PASS(DeadStoreEliminationPass, dse)
    .set_min_opt_level(2)
    .set_stages(PassStage::Opt)
    .Requires("store_comb");
