#include "back/aarch32/varalloc.h"

using namespace mimic::back::aarch32;

namespace {

using VarPos = VarAllocation::VarPos;
using Reg = ARMv7Reg;

}  // namespace

void VarAllocation::Reset() {
  live_intervals_.clear();
  var_pos_.clear();
  cur_pos_ = 0;
  last_call_pos_ = 0;
  saved_reg_.clear();
  local_slot_count_ = 0;
  max_arg_count_ = -1;
}

void VarAllocation::InitFreeReg() {
  free_reg_.clear();
  int end = max_arg_count_ + 1;
  for (auto r = static_cast<int>(Reg::r7); r >= end; --r) {
    free_reg_.push_back(static_cast<Reg>(r));
  }
}

void VarAllocation::LogLiveInterval(const mid::SSAPtr &var) {
  // TODO
  // if (IsGlobalVar(var)) return;

  auto it = live_intervals_.find(var);
  if (it != live_intervals_.end()) {
    auto &info = it->second;

    info.end_pos = cur_pos_;

    if (last_call_pos_ > info.start_pos && last_call_pos_ < cur_pos_) {
      info.must_preserve = true;
    }
  }
  live_intervals_.insert({var, {cur_pos_, cur_pos_, true, false}});
}


void VarAllocation::RunVarAlloc() {
  IntervalStartMap start_map;
  // build map for all normal variables
  for (const auto &i : live_intervals_) {
    if (!i.second.can_alloc_reg) {
      // allocate a new slot now
      var_pos_.insert({i.first, GetNewSlot()});
    }
    else if (!i.second.must_preserve) {
      // insert to start map
      start_map.insert({&i.second, i.first});
    }
  }
  // run first allocation
  InitFreeReg();
  LinearScanAlloc(start_map);
  // // build map for all preserved variables
  // start_map.clear();
  // for (const auto &i : live_intervals_) {
  //   if (i.second.must_preserve) {
  //     // insert to start map
  //     start_map.insert({&i.second, i.first});
  //   }
  // }
  // // run second allocation
  // InitFreeReg(true);
  // LinearScanAlloc(start_map, true);
}

void VarAllocation::LinearScanAlloc(const IntervalStartMap &start_map) {
  IntervalEndMap active;
  for (const auto &i : start_map) {
    ExpireOldIntervals(active, i.first);
    if (free_reg_.empty()) {
      // need to spill
      SpillAtInterval(active, i.first, i.second);
    }
    else {
      // allocate to a free register
      auto reg = free_reg_.back();
      free_reg_.pop_back();
      var_pos_[i.second] = reg;
      // add to active
      active.insert({i.first, i.second});
    }
  }
}

void VarAllocation::ExpireOldIntervals(IntervalEndMap &active,
                                           const LiveInterval *i) {
  for (auto it = active.begin(); it != active.end();) {
    if (it->first->end_pos >= i->start_pos) return;
    // free current element's register
    const auto &pos = var_pos_[it->second];
    if (auto reg = std::get_if<ARMv7Reg>(&pos)) {
      free_reg_.push_back(*reg);
    }
    // remove current element from active
    it = active.erase(it);
  }
}

void VarAllocation::SpillAtInterval(IntervalEndMap &active,
                                        const LiveInterval *i,
                                        const mid::SSAPtr &var) {
  // get last element of active
  auto spill = active.end();
  --spill;
  // check if can allocate register to var
  if (spill->first->end_pos > i->end_pos) {
    // allocate register of spilled variable to i
    assert(std::holds_alternative<ARMv7Reg>(var_pos_[spill->second]));
    var_pos_[var] = var_pos_[spill->second];
    // allocate a slot to spilled variable
    var_pos_[spill->second] = GetNewSlot();
    // remove spill from active
    active.erase(spill);
    // add i to active
    active.insert({i, var});
  }
  else {
    // just allocate a new slot
    var_pos_[var] = GetNewSlot();
  }
}

std::size_t VarAllocation::GetNewSlot() { return local_slot_count_++; }

bool VarAllocation::RunOnFunction(const mid::UserPtr &func) {
  return false;
}

bool VarAllocation::RunOnFunction(const mid::FunctionSSA &func) {
  // func
  Reset();
  // get live intervals & argument info
  for (auto &it : func) {
    it.value()->RunPass(*this);
    ++cur_pos_;
  }
  // run allocation
  RunVarAlloc();
  return false;
}

void VarAllocation::RunOn(mid::BinarySSA &ssa) {
  LogLiveInterval(ssa[0].value());    // lhs
  LogLiveInterval(ssa[1].value());    // rhs
  LogLiveInterval(current_ssaptr);    // dest
}

void VarAllocation::RunOn(mid::UnarySSA &ssa) {
  LogLiveInterval(ssa[1].value());
}

void VarAllocation::RunOn(mid::LoadSSA &ssa) {
  LogLiveInterval(ssa[0].value());
  LogLiveInterval(current_ssaptr);    // dest
}

void VarAllocation::RunOn(mid::StoreSSA &ssa) {
  LogLiveInterval(ssa[0].value());
  LogLiveInterval(ssa[1].value());
}

void VarAllocation::RunOn(mid::AllocaSSA &ssa) {
  LogLiveInterval(current_ssaptr);
}

void VarAllocation::RunOn(mid::BranchSSA &ssa) {
  LogLiveInterval(ssa[0].value());
}

void VarAllocation::RunOn(mid::CallSSA &ssa) {
  // log a function call
  last_call_pos_ = cur_pos_;
  LogLiveInterval(ssa[0].value());
}

void VarAllocation::RunOn(mid::ReturnSSA &ssa) {
  LogLiveInterval(ssa[0].value());
}

void VarAllocation::RunOn(mid::ArgRefSSA &ssa) {
  max_arg_count_ =
      max_arg_count_ < ssa.index() ? ssa.index() : max_arg_count_;
  std::cout << max_arg_count_ << std::endl;
}

void VarAllocation::RunOn(mid::BlockSSA &ssa) {
  for (const auto i : ssa.insts()) {
    current_ssaptr = i;
    i->RunPass(*this);
  }
}
