#include <unordered_map>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/passes/helper/cast.h"
// #include "opt/passes/helper/alloca.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

// get all trivial alloca instructions
// i.e. alloca instructions that have never been addressed
class TrivialAllocaHelperPass : public HelperPass {
 public:
  // scan for all alloca instructions
  // return true if there are trivial allocas
  bool ScanAlloca(BlockSSA *entry) {
    triv_allocas_.clear();
    for (const auto &i : entry->insts()) i->RunPass(*this);
    return !triv_allocas_.empty();
  }

  // check if specific value is a trivial alloca instruction
  bool IsTrivial(const SSAPtr &value) const {
    return triv_allocas_.find(value.get()) != triv_allocas_.end();
  }

  // check if specific value is a trivial & store-only alloca instruction
  bool IsStoreOnly(const SSAPtr &value) const {
    auto it = triv_allocas_.find(value.get());
    return it != triv_allocas_.end() && it->second;
  }

  void RunOn(AllocaSSA &ssa) override {
    bool is_store_only = true;
    is_triv_ = true;
    // traverse on all users
    for (const auto &i : ssa.uses()) {
      is_store_ = false;
      i->user()->RunPass(*this);
      if (!is_triv_) return;
      if (is_store_only && !is_store_) is_store_only = false;
    }
    // add to set
    triv_allocas_.insert({&ssa, is_store_only});
  }

  void RunOn(AccessSSA &ssa) override {
    is_triv_ = false;
  }

  void RunOn(StoreSSA &ssa) override {
    is_store_ = true;
  }

 private:
  bool is_triv_, is_store_;
  std::unordered_map<Value *, bool> triv_allocas_;
};

/*
  a naive memory related local value numbering
  this pass will:
  1.  eliminate redundant load
  2.  eliminate allocas that have only been stored
*/
class MemLVNPass : public FunctionPass {
 public:
  MemLVNPass() {}

  bool RunOnFunction(const UserPtr &func) override {
    if (func->empty()) return false;
    changed_ = false;
    // get all trivial allocas
    auto entry = SSACast<BlockSSA>((*func)[0].value().get());
    if (!helper_.ScanAlloca(entry)) return false;
    // traverse all blocks
    for (const auto &i : *func) i.value()->RunPass(*this);
    // remove all store-only allocas
    auto &insts = entry->insts();
    for (auto it = insts.begin(); it != insts.end();) {
      if (helper_.IsStoreOnly(*it)) {
        it = insts.erase(it);
      }
      else {
        ++it;
      }
    }
    return changed_;
  }

  void RunOn(BlockSSA &ssa) override {
    // traverse all instructions
    auto &insts = ssa.insts();
    for (auto it = insts.begin(); it != insts.end();) {
      remove_flag_ = false;
      (*it)->RunPass(*this);
      // check if need to remove
      if (remove_flag_) {
        if (!changed_) changed_ = true;
        it = insts.erase(it);
      }
      else {
        ++it;
      }
    }
    // release resources
    defs_.clear();
  }

  void RunOn(LoadSSA &ssa) override {
    auto it = defs_.find(ssa.ptr().get());
    if (it != defs_.end()) {
      ssa.ReplaceBy(it->second);
      remove_flag_ = true;
    }
  }

  void RunOn(StoreSSA &ssa) override {
    if (helper_.IsStoreOnly(ssa.ptr())) {
      remove_flag_ = true;
    }
    else if (helper_.IsTrivial(ssa.ptr())) {
      defs_[ssa.ptr().get()] = ssa.value();
    }
  }

 private:
  bool changed_, remove_flag_;
  TrivialAllocaHelperPass helper_;
  std::unordered_map<Value *, SSAPtr> defs_;
};

}  // namespace

// register current pass
REGISTER_PASS(MemLVNPass, mem_lvn, 1, PassStage::PostOpt);
