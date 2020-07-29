#ifndef MIMIC_BACK_ASM_MIR_PASSES_FASTALLOC_H_
#define MIMIC_BACK_ASM_MIR_PASSES_FASTALLOC_H_

#include <list>
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
    Reset();
    for (const auto &i : insts) {
      for (auto &&use : i->oprs()) {
        if (use.value()->IsVirtual()) {
          use.set_value(Allocate(use.value(), func_label));
        }
      }
      if (i->dest() && i->dest()->IsVirtual()) {
        i->set_dest(Allocate(i->dest(), func_label));
      }
    }
  }

  void AddAvaliableReg(const OprPtr &reg) override {
    avaliable_regs_.push_back(reg);
  }

 private:
  void Reset() {
    while (!unused_regs_.empty()) unused_regs_.pop();
    for (const auto &i : avaliable_regs_) unused_regs_.push(i);
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

  // all avaliable architecture registers
  std::list<OprPtr> avaliable_regs_;
  // unused architecture regsters
  std::queue<OprPtr> unused_regs_;
  // allocated virtual registers
  std::unordered_map<OprPtr, OprPtr> allocated_vregs_;
};

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASSES_FASTALLOC_H_
