#ifndef MIMIC_OPT_HELPER_BLKITER_H_
#define MIMIC_OPT_HELPER_BLKITER_H_

#include <cassert>
#include <unordered_set>
#include <queue>
#include <stack>

#include "opt/pass.h"

namespace mimic::opt {
namespace __impl {

// traverse blocks in BFS order
class BFSTraverseHelperPass : public HelperPass {
 public:
  // iterator class
  class Iter {
   public:
    // switch to next block
    Iter &operator++() {
      assert(parent_);
      parent_->NextBlock();
      return *this;
    }

    // check if two iterators are equal
    // NOTE:  can only be used when comparing with the 'end' iterator
    bool operator!=(const Iter &other) const {
      assert(parent_ && !other.parent_);
      return parent_->block_;
    }

    // get current block
    mid::BlockSSA *&operator*() const { return parent_->block_; }

   private:
    friend class BFSTraverseHelperPass;
    Iter(BFSTraverseHelperPass *parent) : parent_(parent) {}
    BFSTraverseHelperPass *parent_;
  };

  BFSTraverseHelperPass(mid::BlockSSA *entry)
      : block_(entry), visited_({entry}) {}

  void RunOn(mid::BranchSSA &ssa) override;
  void RunOn(mid::JumpSSA &ssa) override;

  // iterator method
  Iter begin() { return Iter(this); }
  Iter end() { return Iter(nullptr); }

 private:
  // switch to next block
  void NextBlock();
  // push block to queue if not visited
  void Push(const mid::SSAPtr &block);

  mid::BlockSSA *block_;
  std::unordered_set<mid::BlockSSA *> visited_;
  std::queue<mid::BlockSSA *> queue_;
};

// traverse blocks in DFS order
class DFSTraverseHelperPass : public HelperPass {
 public:
  // iterator class
  class Iter {
   public:
    // switch to next block
    Iter &operator++() {
      assert(parent_);
      parent_->NextBlock();
      return *this;
    }

    // check if two iterators are equal
    // NOTE:  can only be used when comparing with the 'end' iterator
    bool operator!=(const Iter &other) const {
      assert(parent_ && !other.parent_);
      return parent_->cur_block_;
    }

    // get current block
    mid::BlockSSA *&operator*() const { return parent_->cur_block_; }

   private:
    friend class DFSTraverseHelperPass;
    Iter(DFSTraverseHelperPass *parent) : parent_(parent) {}
    DFSTraverseHelperPass *parent_;
  };

  DFSTraverseHelperPass(mid::BlockSSA *entry)
      : cur_block_(entry), visited_({entry}) {}

  void RunOn(mid::BranchSSA &ssa) override;
  void RunOn(mid::JumpSSA &ssa) override;

  // iterator method
  Iter begin() { return Iter(this); }
  Iter end() { return Iter(nullptr); }

 private:
  // visit next block
  void NextBlock();
  // push block to queue if not visited
  void Push(const mid::SSAPtr &block);

  mid::BlockSSA *cur_block_;
  std::unordered_set<mid::BlockSSA *> visited_;
  std::stack<mid::BlockSSA *> blocks_;
};

// traverse blocks in reverse post order
class RPOTraverseHelperPass : public HelperPass {
 public:
  RPOTraverseHelperPass(mid::BlockSSA *entry) { TraverseRPO(entry); }

  void RunOn(mid::BranchSSA &ssa) override;
  void RunOn(mid::JumpSSA &ssa) override;

  // iterator method
  auto begin() const { return rpo_.begin(); }
  auto end() const { return rpo_.end(); }

 private:
  void TraverseRPO(mid::BlockSSA *cur);

  std::list<mid::BlockSSA *> rpo_;
  std::unordered_set<mid::BlockSSA *> visited_;
};

}  // namespace __impl

// traverse blocks in BFS order
inline __impl::BFSTraverseHelperPass BFSTraverse(mid::BlockSSA *entry) {
  return __impl::BFSTraverseHelperPass(entry);
}
inline __impl::BFSTraverseHelperPass BFSTraverse(mid::BlockSSA &entry) {
  return __impl::BFSTraverseHelperPass(&entry);
}
inline __impl::BFSTraverseHelperPass BFSTraverse(
    const mid::BlockPtr &entry) {
  return __impl::BFSTraverseHelperPass(entry.get());
}

// traverse blocks in DFS order
inline __impl::DFSTraverseHelperPass DFSTraverse(mid::BlockSSA *entry) {
  return __impl::DFSTraverseHelperPass(entry);
}
inline __impl::DFSTraverseHelperPass DFSTraverse(mid::BlockSSA &entry) {
  return __impl::DFSTraverseHelperPass(&entry);
}
inline __impl::DFSTraverseHelperPass DFSTraverse(
    const mid::BlockPtr &entry) {
  return __impl::DFSTraverseHelperPass(entry.get());
}

// traverse blocks in reverse post order
inline __impl::RPOTraverseHelperPass RPOTraverse(mid::BlockSSA *entry) {
  return __impl::RPOTraverseHelperPass(entry);
}
inline __impl::RPOTraverseHelperPass RPOTraverse(mid::BlockSSA &entry) {
  return __impl::RPOTraverseHelperPass(&entry);
}
inline __impl::RPOTraverseHelperPass RPOTraverse(
    const mid::BlockPtr &entry) {
  return __impl::RPOTraverseHelperPass(entry.get());
}

}  // namespace mimic::opt

#endif  // MIMIC_OPT_HELPER_BLKITER_H_
