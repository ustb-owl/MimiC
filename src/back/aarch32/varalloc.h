#ifndef MIMIC_BACK_AARCH32_VARALLOC_H_
#define MIMIC_BACK_AARCH32_VARALLOC_H_

#include <cassert>
#include <cstddef>
#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <variant>
#include <vector>

#include "back/aarch32/armv7asm.h"
#include "mid/ssa.h"
#include "opt/pass.h"

namespace mimic::back::aarch32 {
class VarAllocation : public opt::FunctionPass {
 public:
  using VarPos = std::variant<std::size_t, ARMv7Reg>;
  using SavedSet = std::set<ARMv7Reg, std::greater<ARMv7Reg>>;

  VarAllocation() {}
  bool RunOnFunction(const mid::UserPtr &func) override;
  bool RunOnFunction(const mid::FunctionSSA &func);

  void RunOn(mid::BinarySSA &ssa) override;
  void RunOn(mid::UnarySSA &ssa) override;
  void RunOn(mid::LoadSSA &ssa) override;
  void RunOn(mid::StoreSSA &ssa) override;
  void RunOn(mid::AllocaSSA &ssa) override;
  void RunOn(mid::BranchSSA &ssa) override;
  void RunOn(mid::CallSSA &ssa) override;
  void RunOn(mid::ReturnSSA &ssa) override;
  void RunOn(mid::BlockSSA &ssa) override;
  void RunOn(mid::ArgRefSSA &ssa) override;
  // void RunOn(mid::Value &ssa);

  const VarPos &GetPosition(const mid::SSAPtr &var) const {
    auto it = var_pos_.find(var);
    assert(it != var_pos_.end());
    return it->second;
  }

  std::size_t save_area_size() const { return saved_reg_.size() * 4; }
  const SavedSet &saved_reg() const { return saved_reg_; }
  std::size_t local_area_size() const { return local_slot_count_ * 4; }
  std::size_t arg_area_size() const { return max_arg_count_ * 4; }

 private:
  struct LiveInterval {
    std::size_t start_pos;
    std::size_t end_pos;
    bool can_alloc_reg;
    bool must_preserve;
  };

  struct LiveIntervalStartCmp {
    bool operator()(const LiveInterval *lhs,
                    const LiveInterval *rhs) const {
      return lhs->start_pos < rhs->start_pos;
    }
  };

  struct LiveIntervalEndCmp {
    bool operator()(const LiveInterval *lhs,
                    const LiveInterval *rhs) const {
      return lhs->end_pos < rhs->end_pos;
    }
  };

  using IntervalStartMap =
      std::multimap<const LiveInterval *, mid::SSAPtr, LiveIntervalStartCmp>;
  using IntervalEndMap =
      std::multimap<const LiveInterval *, mid::SSAPtr, LiveIntervalEndCmp>;

  void Reset();
  void InitFreeReg();
  void LogLiveInterval(const mid::SSAPtr &var);
  void RunVarAlloc();
  void LinearScanAlloc(const IntervalStartMap &start_map);
  void ExpireOldIntervals(IntervalEndMap &active, const LiveInterval *i);
  void SpillAtInterval(IntervalEndMap &active, const LiveInterval *i,
                       const mid::SSAPtr &var);
  std::size_t GetNewSlot();

  std::unordered_map<mid::SSAPtr, LiveInterval> live_intervals_;
  std::unordered_map<mid::SSAPtr, VarPos> var_pos_;
  std::vector<ARMv7Reg> free_reg_;
  std::size_t cur_pos_;
  std::size_t last_call_pos_;
  SavedSet saved_reg_;
  std::size_t local_slot_count_;
  int max_arg_count_;

  mid::SSAPtr current_ssaptr;
};
}  // namespace mimic::back::aarch32

#endif  // MIMIC_BACK_AARCH32_VARALLOC_H_
