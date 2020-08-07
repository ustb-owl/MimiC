#include "opt/helper/dom.h"

#include "opt/helper/cast.h"
#include "opt/helper/blkiter.h"

using namespace mimic::mid;
using namespace mimic::opt;
using namespace mimic::utils;

void DominanceChecker::SolveDominance(FunctionSSA *func) {
  auto entry = SSACast<BlockSSA>(func->entry().get());
  auto rpo = RPOTraverse(entry);
  // number all of the blocks in current function
  std::size_t cur_id = 0;
  for (const auto &i : rpo) block_id_[i] = cur_id++;
  // get range of iteration, skip entry block
  auto begin = ++rpo.begin(), end = rpo.end();
  // initialize dominance set
  dom_[entry].Resize(cur_id);
  dom_[entry].Set(0);
  for (auto it = begin; it != end; ++it) {
    dom_[*it].Resize(cur_id);
    dom_[*it].Fill(1);
  }
  // run solver
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto it = begin; it != end; ++it) {
      BitVec temp(cur_id);
      temp.Fill(1);
      for (const auto &pred : **it) {
        auto pred_ptr = SSACast<BlockSSA>(pred.value().get());
        if (IsDeadBlock(pred_ptr)) continue;
        temp &= dom_[pred_ptr];
      }
      BitVec self_set(cur_id);
      self_set.Set(block_id_[*it]);
      temp |= self_set;
      // update dominance
      if (temp != dom_[*it]) {
        dom_[*it] = temp;
        changed = true;
      }
    }
  }
}

bool DominanceChecker::IsDominate(mid::BlockSSA *b1,
                                  mid::BlockSSA *b2) const {
  auto dom_it = dom_.find(b2);
  auto id_it = block_id_.find(b1);
  assert(dom_it != dom_.end() && id_it != block_id_.end());
  return dom_it->second.Get(id_it->second);
}
