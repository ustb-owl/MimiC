#include "opt/helper/blkiter.h"

#include "opt/helper/cast.h"

using namespace mimic::mid;
using namespace mimic::opt::__impl;

void BFSTraverseHelperPass::NextBlock() {
  // try to push successors to queue
  block_->insts().back()->RunPass(*this);
  // update current block
  if (queue_.empty()) {
    block_ = nullptr;
  }
  else {
    block_ = queue_.front();
    queue_.pop();
  }
}

void BFSTraverseHelperPass::Push(const SSAPtr &block) {
  auto ptr = SSACast<BlockSSA>(block.get());
  if (visited_.insert(ptr).second) queue_.push(ptr);
}

void BFSTraverseHelperPass::RunOn(BranchSSA &ssa) {
  // push true/false block to queue
  Push(ssa.true_block());
  Push(ssa.false_block());
}

void BFSTraverseHelperPass::RunOn(JumpSSA &ssa) {
  // push target block to queue
  Push(ssa.target());
}

void DFSTraverseHelperPass::NextBlock() {
  // try to push successors to queue
  cur_block_->insts().back()->RunPass(*this);
  // update current block
  if (blocks_.empty()) {
    cur_block_ = nullptr;
  }
  else {
    cur_block_ = blocks_.top();
    blocks_.pop();
  }
}

void DFSTraverseHelperPass::Push(const SSAPtr &block) {
  auto ptr = SSACast<BlockSSA>(block.get());
  if (visited_.insert(ptr).second) blocks_.push(ptr);
}

void DFSTraverseHelperPass::RunOn(BranchSSA &ssa) {
  // push true/false block to stack
  Push(ssa.false_block());
  Push(ssa.true_block());
}

void DFSTraverseHelperPass::RunOn(JumpSSA &ssa) {
  // push target block to stack
  Push(ssa.target());
}

void RPOTraverseHelperPass::TraverseRPO(BlockSSA *cur) {
  if (!visited_.insert(cur).second) return;
  // try to visit successors
  cur->insts().back()->RunPass(*this);
  rpo_.push_front(cur);
}

void RPOTraverseHelperPass::RunOn(BranchSSA &ssa) {
  // visit true/false block
  TraverseRPO(SSACast<BlockSSA>(ssa.true_block().get()));
  TraverseRPO(SSACast<BlockSSA>(ssa.false_block().get()));
}

void RPOTraverseHelperPass::RunOn(JumpSSA &ssa) {
  // visit target
  TraverseRPO(SSACast<BlockSSA>(ssa.target().get()));
}
