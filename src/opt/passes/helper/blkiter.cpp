#include "opt/passes/helper/blkiter.h"

#include "opt/passes/helper/cast.h"

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
