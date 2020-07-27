#ifndef MIMIC_BACK_ASM_MIR_PASSES_LINEARSCAN_H_
#define MIMIC_BACK_ASM_MIR_PASSES_LINEARSCAN_H_

#include <map>
#include <vector>
#include <queue>
#include <unordered_map>
#include <cstddef>
#include <cassert>

#include "back/asm/mir/passes/regalloc.h"

namespace mimic::back::asmgen {

/*
  linear scan register allocator
  reference: M. Poletto, V. Sarkar, Linear Scan Register Allocation
*/
class LinearScanRegAllocPass : public RegAllocatorBase {
 public:
  LinearScanRegAllocPass() {}

  void RunOn(InstPtrList &insts) override {
    Reset();
    // perform allocation
    InitLiveIntervals(insts);
    LinearScanAlloc();
    // apply to virtual registers
    for (const auto &i : insts) {
      for (auto &&opr : i->oprs()) {
        if (opr.value()->IsVirtual()) {
          opr.set_value(GetAllocatedOpr(opr.value()));
        }
      }
      if (i->dest() && i->dest()->IsVirtual()) {
        i->set_dest(GetAllocatedOpr(i->dest()));
      }
    }
  }

  void AddAvaliableReg(const OprPtr &reg) override {
    avaliable_regs_.push_back(reg);
  }

 private:
  // live interval information
  struct LiveInterval {
    std::size_t start_pos;
    std::size_t end_pos;
  };

  // map of live intervals in order of increasing start position
  struct LiveIntervalStartCmp {
    bool operator()(const LiveInterval *lhs,
                    const LiveInterval *rhs) const {
      return lhs->start_pos < rhs->start_pos;
    }
  };
  using IntervalStartMap =
      std::multimap<const LiveInterval *, OprPtr, LiveIntervalStartCmp>;

  // map of live intervals in order of increasing end position
  struct LiveIntervalEndCmp {
    bool operator()(const LiveInterval *lhs,
                    const LiveInterval *rhs) const {
      return lhs->end_pos < rhs->end_pos;
    }
  };
  using IntervalEndMap =
      std::multimap<const LiveInterval *, OprPtr, LiveIntervalEndCmp>;

  // reset for next run
  void Reset() {
    // clear free register/slot pool
    while (!free_regs_.empty()) free_regs_.pop();
    while (!free_slots_.empty()) free_slots_.pop();
    // initialize free registers
    for (const auto &i : avaliable_regs_) free_regs_.push(i);
    // reset other stuffs
    live_intervals_.clear();
    vregs_.clear();
  }

  // initialize live interval info
  void InitLiveIntervals(InstPtrList &insts) {
    std::size_t pos = 0;
    live_intervals_.clear();
    // traverse all instructions
    for (const auto &i : insts) {
      if (i->dest()) LogLiveInterval(i->dest(), pos);
      for (const auto &opr : i->oprs()) {
        LogLiveInterval(opr.value(), pos);
      }
      ++pos;
    }
  }

  // log position info of the specific value
  void LogLiveInterval(const OprPtr &opr, std::size_t cur_pos) {
    if (!opr->IsVirtual()) return;
    // get live interval info
    auto it = live_intervals_.find(opr);
    if (it != live_intervals_.end()) {
      auto &info = it->second;
      // update end position
      info.end_pos = cur_pos;
    }
    else {
      // add new live interval info
      live_intervals_.insert({opr, {cur_pos, cur_pos}});
    }
  }

  // perform linear scan register allocation
  void LinearScanAlloc() {
    IntervalStartMap start_map;
    IntervalEndMap active;
    // build up live interval map
    for (const auto &i : live_intervals_) {
      start_map.insert({&i.second, i.first});
    }
    // perform allocation
    for (const auto &i : start_map) {
      ExpireOldIntervals(active, i.first);
      if (!free_regs_.empty()) {
        // allocate a free register
        auto reg = free_regs_.front();
        free_regs_.pop();
        vregs_[i.second] = reg;
        MarkAsAllocated(reg);
        // add to active
        active.insert({i.first, i.second});
      }
      else if (!free_slots_.empty()) {
        // allocate a free slot
        auto slot = free_slots_.front();
        free_slots_.pop();
        vregs_[i.second] = slot;
        // add to active
        active.insert({i.first, i.second});
      }
      else {
        // need to spill
        SpillAtInterval(active, i.first, i.second);
      }
    }
  }

  void ExpireOldIntervals(IntervalEndMap &active, const LiveInterval *i) {
    for (auto it = active.begin(); it != active.end();) {
      if (it->first->end_pos >= i->start_pos) return;
      // free current element's register/slot
      const auto &opr = vregs_[it->second];
      if (opr->IsReg()) {
        free_regs_.push(opr);
      }
      else {
        assert(opr->IsSlot());
        free_slots_.push(opr);
      }
      // remove current element from active
      it = active.erase(it);
    }
  }

  void SpillAtInterval(IntervalEndMap &active, const LiveInterval *i,
                       const OprPtr &opr) {
    // get last element of active
    auto spill = --active.end();
    // check if can allocate register to var
    if (spill->first->end_pos > i->end_pos) {
      // allocate register/slot of spilled value to i
      vregs_[opr] = vregs_[spill->second];
      // allocate a slot to spilled value
      vregs_[spill->second] = AllocSlot();
      // remove spill from active
      active.erase(spill);
      // add i to active
      active.insert({i, opr});
    }
    else {
      // just allocate a new slot
      vregs_[opr] = AllocSlot();
    }
  }

  // return allocated register/slot of the specific virtual register
  const OprPtr &GetAllocatedOpr(const OprPtr &vreg) {
    assert(vreg->IsVirtual() && vregs_.find(vreg) != vregs_.end());
    return vregs_[vreg];
  }

  // all avaliable registers
  std::vector<OprPtr> avaliable_regs_;
  // free register/slot pool (queue)
  std::queue<OprPtr> free_regs_, free_slots_;
  // live intervals
  std::unordered_map<OprPtr, LiveInterval> live_intervals_;
  // allocated registers/slots of all virtual registers
  std::unordered_map<OprPtr, OprPtr> vregs_;
};

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASSES_LINEARSCAN_H_
