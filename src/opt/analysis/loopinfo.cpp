#include "opt/analysis/loopinfo.h"

#include <stack>
#include <cassert>

#include "opt/passman.h"
#include "opt/helper/cast.h"
#include "opt/analysis/dominance.h"
#include "mid/module.h"

using namespace mimic::mid;
using namespace mimic::opt;

// register current pass
REGISTER_PASS(LoopInfoPass, loop_info)
    .set_is_analysis(true)
    .Requires("dom_info");


void LoopInfoPass::ScanOn(const FuncPtr &func) {
  // scan for back edges
  auto &loops = loops_[func.get()];
  for (const auto &i : *func) {
    auto block = SSACast<BlockSSA>(i.value().get());
    // skip dead blocks
    const auto &dom = PassManager::GetPass<DominanceInfoPass>("dom_info");
    if (dom.IsDeadBlock(block)) continue;
    // check all incoming edges (pred -> block)
    for (const auto &p : *block) {
      auto pred = SSACast<BlockSSA>(p.value().get());
      if (!dom.IsDeadBlock(pred) && dom.IsDominate(block, pred)) {
        // back edge found
        ScanNaturalLoop(loops, pred, block);
      }
    }
  }
}

void LoopInfoPass::ScanNaturalLoop(LoopInfoList &loops, BlockSSA *be_tail,
                                   BlockSSA *be_head) {
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
  // scan for preheader
  ScanPreheader(info);
  // scan for exit blocks
  ScanExitBlocks(info);
  // scan for induction variable info
  ScanIndVarInfo(info);
  // add to list
  loops.push_back(std::move(info));
}

void LoopInfoPass::ScanPreheader(LoopInfo &info) {
  if (info.entry->size() == 2) {
    if ((*info.entry)[0].value().get() == info.tail) {
      info.preheader = SSACast<BlockSSA>((*info.entry)[1].value().get());
    }
    else {
      assert((*info.entry)[1].value().get() == info.tail);
      info.preheader = SSACast<BlockSSA>((*info.entry)[0].value().get());
    }
  }
  else {
    info.preheader = nullptr;
  }
}

void LoopInfoPass::ScanExitBlocks(LoopInfo &info) {
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
}

void LoopInfoPass::ScanIndVarInfo(LoopInfo &info) {
  // the entry of loop must have only two predecessors
  if (info.entry->size() != 2) return;
  // loop must have only one exit block, and it must be entry block
  if (info.exit.size() != 1 || !info.exit.count(info.entry)) return;
  // get condition
  auto branch = SSACast<BranchSSA>(info.entry->insts().back().get());
  auto bin = SSADynCast<BinarySSA>(branch->cond().get());
  if (!bin) return;
  info.end_cond = bin;
  // get exit block
  auto tb = SSACast<BlockSSA>(branch->true_block().get());
  auto fb = SSACast<BlockSSA>(branch->false_block().get());
  info.exit_block = info.body.count(tb) ? fb : tb;
  info.body_block = info.body.count(tb) ? tb : fb;
  // get induction variable
  for (const auto &i : info.entry->insts()) {
    if (auto phi = SSADynCast<PhiSSA>(i.get())) {
      for (const auto &use : phi->uses()) {
        if (use->user() == info.end_cond) {
          info.ind_var = phi;
          break;
        }
      }
    }
  }
  if (!info.ind_var) return;
  // get initial value & modifier
  assert(info.ind_var->size() == 2);
  for (const auto &i : *info.ind_var) {
    auto opr = SSACast<PhiOperandSSA>(i.value().get());
    if (opr->block().get() != info.tail) {
      info.init_val = opr->value().get();
    }
    else {
      // modifier must be a binary instruction
      info.modifier = SSADynCast<BinarySSA>(opr->value().get());
    }
  }
}

bool LoopInfoPass::RunOnFunction(const FuncPtr &func) {
  if (func->is_decl()) return false;
  ScanOn(func);
  return false;
}

void LoopInfoPass::Initialize() {
  loops_.clear();
}
