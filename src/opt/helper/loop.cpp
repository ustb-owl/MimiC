#include "opt/helper/loop.h"

#include <stack>
#include <cassert>

#include "opt/helper/cast.h"
#include "mid/module.h"

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


BlockPtr PreheaderCreator::CreatePreheader(const LoopInfo &loop) {
  // check if need to exit
  if (loop.entry->size() == 2) {
    if ((*loop.entry)[0].value().get() == loop.tail) {
      return SSACast<BlockSSA>((*loop.entry)[1].value());
    }
    else {
      assert((*loop.entry)[1].value().get() == loop.tail);
      return SSACast<BlockSSA>((*loop.entry)[0].value());
    }
  }
  // create preheader block
  auto mod = MakeModule(loop.entry->logger());
  preheader_ = mod.CreateBlock(loop.entry->parent(),
                               loop.entry->name() + ".preheader");
  // set up predecessors
  for (auto &&i : *loop.entry) {
    if (i.value().get() != loop.tail) {
      preheader_->AddValue(i.value());
      i.set_value(nullptr);
    }
  }
  loop.entry->RemoveValue(nullptr);
  loop.entry->AddValue(preheader_);
  // reroute predecessor's successor
  loop_entry_ = loop.entry;
  for (const auto &i : *preheader_) {
    auto pred = SSACast<BlockSSA>(i.value().get());
    pred->insts().back()->RunPass(*this);
  }
  // create jump to loop entry
  assert(!loop.entry->uses().empty());
  const auto &entry_ptr = loop.entry->uses().front()->value();
  auto jump = std::make_shared<JumpSSA>(entry_ptr);
  jump->set_logger(loop.entry->logger());
  preheader_->insts().push_back(std::move(jump));
  // reroute phi nodes in loop header
  ReroutePhi(loop);
  return preheader_;
}

void PreheaderCreator::ReroutePhi(const LoopInfo &loop) {
  if (preheader_->size() == 1) {
    // phi only has two operands, update incoming block
    for (const auto &i : loop.entry->insts()) {
      if (auto phi = SSADynCast<PhiSSA>(i.get())) {
        for (const auto &opr : *phi) {
          auto opr_ptr = SSACast<PhiOperandSSA>(opr.value().get());
          if (opr_ptr->block().get() != loop.tail) {
            opr_ptr->set_block(preheader_);
            break;
          }
        }
      }
    }
  }
  else {
    assert(preheader_->size() > 1);
    // phi has more than two operands, place a new phi in preheader
    for (const auto &i : loop.entry->insts()) {
      if (auto phi = SSADynCast<PhiSSA>(i.get())) {
        // extract operands from non-tail block
        SSAPtrList oprs;
        for (auto &&opr : *phi) {
          auto opr_ptr = SSACast<PhiOperandSSA>(opr.value().get());
          if (opr_ptr->block().get() != loop.tail) {
            oprs.push_back(opr.value());
            opr.set_value(nullptr);
          }
        }
        phi->RemoveValue(nullptr);
        // create new phi node
        auto mod = MakeModule(phi->logger());
        auto new_phi = mod.CreatePhi(oprs);
        // add the new phi node to the old one
        phi->AddValue(mod.CreatePhiOperand(new_phi, preheader_));
        // insert the new phi node to preheader
        preheader_->insts().push_front(std::move(new_phi));
      }
    }
  }
}

void PreheaderCreator::RunOn(BranchSSA &ssa) {
  if (ssa.true_block().get() == loop_entry_) {
    ssa.set_true_block(preheader_);
  }
  else {
    assert(ssa.false_block().get() == loop_entry_);
    ssa.set_false_block(preheader_);
  }
}

void PreheaderCreator::RunOn(JumpSSA &ssa) {
  assert(ssa.target().get() == loop_entry_);
  ssa.set_target(preheader_);
}
