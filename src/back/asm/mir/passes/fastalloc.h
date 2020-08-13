#ifndef MIMIC_BACK_ASM_MIR_PASSES_FASTALLOC_H_
#define MIMIC_BACK_ASM_MIR_PASSES_FASTALLOC_H_

#include <queue>
#include <unordered_map>
#include <cassert>

#include "back/asm/mir/passes/regalloc.h"

namespace mimic::back::asmgen {

/*
  fast register allocation
  allocate register if there are remaining registers
  otherwise, allocate a new slot
*/
class FastRegAllocPass : public RegAllocatorBase {
 public:
  FastRegAllocPass() {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    Reset(func_label);
    for (const auto &i : insts) {
      for (auto &&use : i->oprs()) {
        if (use.value()->IsVirtual()) {
          const auto &dest = Allocate(use.value(), func_label);
          AllocateVRegTo(use.value(), dest);
        }
      }
      if (i->dest() && i->dest()->IsVirtual()) {
        const auto &dest = Allocate(i->dest(), func_label);
        AllocateVRegTo(i->dest(), dest);
      }
    }
  }

 private:
  void Reset(const OprPtr &func_label) {
    while (!unused_regs_.empty()) unused_regs_.pop();
    for (const auto &i : GetRegList(func_label)) unused_regs_.push(i);
    allocated_vregs_.clear();
  }

  const OprPtr &Allocate(const OprPtr &vreg, const OprPtr &func_label) {
    assert(vreg->IsVirtual());
    // find in allocated virtual registers
    auto it = allocated_vregs_.find(vreg);
    if (it != allocated_vregs_.end()) {
      return it->second;
    }
    else {
      OprPtr pos;
      // try to allocate a register
      if (!unused_regs_.empty()) {
        pos = unused_regs_.front();
        unused_regs_.pop();
      }
      else {
        // allocate a new slot
        pos = allocator().AllocateSlot(func_label);
      }
      return allocated_vregs_.insert({vreg, pos}).first->second;
    }
  }

  // unused architecture regsters
  std::queue<OprPtr> unused_regs_;
  // allocated virtual registers
  std::unordered_map<OprPtr, OprPtr> allocated_vregs_;
};

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASSES_FASTALLOC_H_
