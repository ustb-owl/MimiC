#ifndef MIMIC_BACK_ASM_MIR_PASSES_REGALLOC_H_
#define MIMIC_BACK_ASM_MIR_PASSES_REGALLOC_H_

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <memory>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/mir/virtreg.h"

namespace mimic::back::asmgen {

// live interval information
struct LiveInterval {
  std::size_t start_pos;
  std::size_t end_pos;
  bool can_alloc_temp;
};

// live intervals in function
using LiveIntervals = std::unordered_map<OprPtr, LiveInterval>;

// live intervals of all functions
using FuncLiveIntervals = std::unordered_map<OprPtr, LiveIntervals>;

// comparator for graph nodes (virtual registers)
struct NodeCompare {
  bool operator()(const OprPtr &n1, const OprPtr &n2) const {
    auto p1 = static_cast<VirtRegOperand *>(n1.get());
    auto p2 = static_cast<VirtRegOperand *>(n2.get());
    return p1->id() < p2->id();
  }
};

// information of graph's node
struct IfGraphNodeInfo {
  std::set<OprPtr, NodeCompare> neighbours;
  std::set<OprPtr, NodeCompare> suggest_same;
  bool can_alloc_temp;
};

// interference graph of a function
using IfGraph = std::map<OprPtr, IfGraphNodeInfo, NodeCompare>;

// interference graph of all functions
using FuncIfGraphs = std::unordered_map<OprPtr, IfGraph>;

// avaliable register list
using RegList = std::vector<OprPtr>;

// avaliable register list reference of all functions
using FuncRegList = std::unordered_map<OprPtr, const RegList *>;

// type of temporary register checker
using TempRegChecker = std::function<bool(const OprPtr &)>;

// slot allocator
class SlotAllocator {
 public:
  SlotAllocator() {}
  SlotAllocator(std::function<OprPtr(const OprPtr &)> alloc_slot)
      : alloc_slot_(alloc_slot) {}

  // allocate a new stack slot
  OprPtr AllocateSlot(const OprPtr &func_label) const {
    return alloc_slot_(func_label);
  }

 private:
  std::function<OprPtr(const OprPtr &)> alloc_slot_;
};

// base class of all register allocators
//
// register allocator should scan all virtual registers
// and update 'alloc_to' property of virtual registers
// to specify where they should be allocated
class RegAllocatorBase : public PassInterface {
 public:
  virtual ~RegAllocatorBase() = default;

  // setters
  // specify reference of avaliable temporary architecture register list
  void set_func_temp_reg_list(const FuncRegList *func_temp_reg_list) {
    func_temp_reg_list_ = func_temp_reg_list;
  }
  // specify reference of avaliable architecture register list
  void set_func_reg_list(const FuncRegList *func_reg_list) {
    func_reg_list_ = func_reg_list;
  }
  // specify stack slot allocator
  void set_allocator(SlotAllocator allocator) { allocator_ = allocator; }
  // specify temporary register checker
  void set_temp_checker(TempRegChecker temp_checker) {
    temp_checker_ = temp_checker;
  }

 protected:
  // get avaliable temporary register list of the specific function
  const RegList &GetTempRegList(const OprPtr &func_label) const {
    auto it = func_temp_reg_list_->find(func_label);
    assert(it != func_temp_reg_list_->end());
    return *it->second;
  }

  // get avaliable register list of the specific function
  const RegList &GetRegList(const OprPtr &func_label) const {
    auto it = func_reg_list_->find(func_label);
    assert(it != func_reg_list_->end());
    return *it->second;
  }

  // check if the specific operand is a temporary register
  bool IsTempReg(const OprPtr &opr) const { return temp_checker_(opr); }

  // specify 'alloc_to' property of the specific
  void AllocateVRegTo(const OprPtr &vreg, const OprPtr &dest) {
    assert(vreg->IsVirtual() &&
           ((dest->IsReg() && !dest->IsVirtual()) || dest->IsSlot()));
    static_cast<VirtRegOperand *>(vreg.get())->set_alloc_to(dest);
  }

  // getters
  const SlotAllocator &allocator() const { return allocator_; }

 private:
  const FuncRegList *func_temp_reg_list_, *func_reg_list_;
  SlotAllocator allocator_;
  TempRegChecker temp_checker_;
};

// pointer to register allocator
using RegAllocPtr = std::unique_ptr<RegAllocatorBase>;

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASSES_REGALLOC_H_
