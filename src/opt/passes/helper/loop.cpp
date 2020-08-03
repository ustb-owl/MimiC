#include "opt/passes/helper/loop.h"

#include <stack>
#include <cassert>

#include "opt/passes/helper/cast.h"

using namespace mimic::mid;
using namespace mimic::opt;

void LoopDetector::ScanOn(DominanceChecker &dom, FunctionSSA *func) {
  // scan for back edges
  for (const auto &i : *func) {
    auto block = SSACast<BlockSSA>(i.value().get());
    // skip dead blocks
    if (dom.IsDeadBlock(block)) continue;
    // check all incoming edges (pred -> block)
    for (const auto &p : *block) {
      auto pred = SSACast<BlockSSA>(p.value().get());
      if (!dom.IsDeadBlock(pred) && dom.IsDominate(block, pred)) {
        // back edge found
        ScanNaturalLoop(pred, block);
      }
    }
  }
}

void LoopDetector::ScanNaturalLoop(BlockSSA *be_tail, BlockSSA *be_head) {
  // initialize loop info
  LoopInfo info = {be_head, be_tail};
  info.body.insert({be_head, be_tail});
  // initialize block stack
  std::stack<BlockSSA *> blocks;
  if (be_tail != be_head) {
    blocks.push(be_tail);
  }
  // add predecessors of tail block that are not predecessors of head block
  // to the set of blocks in the loop, since head dominates tail, this only
  // adds nodes in the loop
  while (!blocks.empty()) {
    auto block = blocks.top();
    blocks.pop();
    for (const auto &p : *block) {
      auto pred = SSACast<BlockSSA>(p.value().get());
      if (info.body.insert(pred).second) {
        blocks.push(pred);
      }
    }
  }
  // scan for exit blocks
  for (const auto &i : info.body) {
    assert(!i->insts().empty());
    if (auto branch = SSADynCast<BranchSSA>(i->insts().back())) {
      auto tb = SSACast<BlockSSA>(branch->true_block().get());
      auto fb = SSACast<BlockSSA>(branch->false_block().get());
      if (!info.body.count(tb) || !info.body.count(fb)) {
        info.exit.insert(i);
      }
    }
  }
  // add to list
  loops_.push_back(std::move(info));
}
