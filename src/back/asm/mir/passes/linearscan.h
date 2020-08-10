#ifndef MIMIC_BACK_ASM_MIR_PASSES_LINEARSCAN_H_
#define MIMIC_BACK_ASM_MIR_PASSES_LINEARSCAN_H_

#include <map>
#include <vector>
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
  // live interval information
  struct LiveInterval {
    std::size_t start_pos;
    std::size_t end_pos;
  };
  // live intervals in function
  using LiveIntervals = std::unordered_map<OprPtr, LiveInterval>;
  // live intervals of all functions
  using FuncLiveIntervals = std::unordered_map<OprPtr, LiveIntervals>;

  LinearScanRegAllocPass() {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    Reset();
    // perform allocation
    LinearScanAlloc(func_label);
    // apply to virtual registers
    for (const auto &i : insts) {
      for (auto &&opr : i->oprs()) {
        if (opr.value()->IsVirtual()) {
          AllocateVRegTo(opr.value(), GetAllocatedOpr(opr.value()));
        }
      }
      if (i->dest() && i->dest()->IsVirtual()) {
        AllocateVRegTo(i->dest(), GetAllocatedOpr(i->dest()));
      }
    }
  }

  void AddAvaliableReg(const OprPtr &reg) override {
    avaliable_regs_.push_back(reg);
  }

  // setter
  void set_func_live_intervals(
      const FuncLiveIntervals *func_live_intervals) {
    func_live_intervals_ = func_live_intervals;
  }

 private:
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
    free_regs_.clear();
    free_slots_.clear();
    // initialize free registers
    for (auto it = avaliable_regs_.rbegin(); it != avaliable_regs_.rend();
         ++it) {
      free_slots_.push_back(*it);
    }
    // reset other stuffs
    vregs_.clear();
  }

  // perform linear scan register allocation
  void LinearScanAlloc(const OprPtr &func_label) {
    IntervalStartMap start_map;
    IntervalEndMap active;
    // build up live interval map
    auto it = func_live_intervals_->find(func_label);
    assert(it != func_live_intervals_->end());
    for (const auto &i : it->second) {
      start_map.insert({&i.second, i.first});
    }
    // perform allocation
    for (const auto &i : start_map) {
      ExpireOldIntervals(active, i.first);
      if (!free_regs_.empty()) {
        // allocate a free register
        auto reg = free_regs_.back();
        free_regs_.pop_back();
        vregs_[i.second] = reg;
        // add to active
        active.insert({i.first, i.second});
      }
      else if (!free_slots_.empty()) {
        // allocate a free slot
        auto slot = free_slots_.back();
        free_slots_.pop_back();
        vregs_[i.second] = slot;
        // add to active
        active.insert({i.first, i.second});
      }
      else {
        // need to spill
        SpillAtInterval(active, i.first, i.second, func_label);
      }
    }
  }

  void ExpireOldIntervals(IntervalEndMap &active, const LiveInterval *i) {
    for (auto it = active.begin(); it != active.end();) {
      if (it->first->end_pos >= i->start_pos) return;
      // free current element's register/slot
      const auto &opr = vregs_[it->second];
      if (opr->IsReg()) {
        free_regs_.push_back(opr);
      }
      else {
        assert(opr->IsSlot());
        free_slots_.push_back(opr);
      }
      // remove current element from active
      it = active.erase(it);
    }
  }

  void SpillAtInterval(IntervalEndMap &active, const LiveInterval *i,
                       const OprPtr &opr, const OprPtr &func_label) {
    // get last element of active
    auto spill = --active.end();
    // check if can allocate register to var
    if (spill->first->end_pos > i->end_pos) {
      // allocate register/slot of spilled value to i
      vregs_[opr] = vregs_[spill->second];
      // allocate a slot to spilled value
      vregs_[spill->second] = allocator().AllocateSlot(func_label);
      // remove spill from active
      active.erase(spill);
      // add i to active
      active.insert({i, opr});
    }
    else {
      // just allocate a new slot
      vregs_[opr] = allocator().AllocateSlot(func_label);
    }
  }

  // return allocated register/slot of the specific virtual register
  const OprPtr &GetAllocatedOpr(const OprPtr &vreg) {
    assert(vreg->IsVirtual() && vregs_.find(vreg) != vregs_.end());
    return vregs_[vreg];
  }

  // all avaliable registers
  std::vector<OprPtr> avaliable_regs_;
  // free register/slot pool
  std::vector<OprPtr> free_regs_, free_slots_;
  // reference of live intervals
  const FuncLiveIntervals *func_live_intervals_;
  // allocated registers/slots of all virtual registers
  std::unordered_map<OprPtr, OprPtr> vregs_;
};

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASSES_LINEARSCAN_H_
