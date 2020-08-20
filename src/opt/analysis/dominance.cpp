#include "opt/analysis/dominance.h"

#include "opt/passman.h"
#include "opt/helper/cast.h"
#include "opt/helper/blkiter.h"

using namespace mimic::mid;
using namespace mimic::opt;
using namespace mimic::utils;

// register current pass
REGISTER_PASS(DominanceInfoPass, dom_info)
    .set_is_analysis(true);


const DominanceInfoPass::DominanceInfo &DominanceInfoPass::GetDominanceInfo(
    BlockSSA *block) const {
  auto parent = block->parent().get();
  auto it = dom_info_.find(parent);
  assert(it != dom_info_.end());
  return it->second;
}

bool DominanceInfoPass::IsDeadBlock(BlockSSA *block) const {
  return !GetDominanceInfo(block).block_id.count(block);
}

bool DominanceInfoPass::IsDominate(BlockSSA *b1, BlockSSA *b2) const {
  assert(b1->parent() == b2->parent());
  const auto &info = GetDominanceInfo(b1);
  auto dom_it = info.dom.find(b2);
  auto id_it = info.block_id.find(b1);
  assert(dom_it != info.dom.end() && id_it != info.block_id.end());
  return dom_it->second.Get(id_it->second);
}

void DominanceInfoPass::SolveDominance(const FuncPtr &func) {
  auto &info = dom_info_[func.get()];
  auto entry = SSACast<BlockSSA>(func->entry().get());
  auto rpo = RPOTraverse(entry);
  // number all of the blocks in current function
  std::size_t cur_id = 0;
  for (const auto &i : rpo) info.block_id[i] = cur_id++;
  // get range of iteration, skip entry block
  auto begin = ++rpo.begin(), end = rpo.end();
  // initialize dominance set
  info.dom[entry].Resize(cur_id);
  info.dom[entry].Set(0);
  for (auto it = begin; it != end; ++it) {
    info.dom[*it].Resize(cur_id);
    info.dom[*it].Fill(1);
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
        temp &= info.dom[pred_ptr];
      }
      BitVec self_set(cur_id);
      self_set.Set(info.block_id[*it]);
      temp |= self_set;
      // update dominance
      if (temp != info.dom[*it]) {
        info.dom[*it] = temp;
        changed = true;
      }
    }
  }
}

bool DominanceInfoPass::RunOnFunction(const FuncPtr &func) {
  if (func->is_decl()) return false;
  SolveDominance(func);
  return false;
}

void DominanceInfoPass::Initialize() { dom_info_.clear(); }
