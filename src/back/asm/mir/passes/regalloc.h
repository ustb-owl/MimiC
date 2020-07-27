#ifndef MIMIC_BACK_ASM_MIR_PASSES_REGALLOC_H_
#define MIMIC_BACK_ASM_MIR_PASSES_REGALLOC_H_

#include <functional>
#include <unordered_set>
#include <cassert>

#include "back/asm/mir/pass.h"

namespace mimic::back::asmgen {

class RegAllocatorBase : public PassInterface {
 public:
  virtual ~RegAllocatorBase() = default;

  // add avaliable architecture register
  virtual void AddAvaliableReg(const OprPtr &reg) = 0;

  // setters
  // specify stack slot allocator
  void set_slot_alloc(std::function<OprPtr()> slot_alloc) {
    slot_alloc_ = slot_alloc;
  }

  // getters
  const std::unordered_set<OprPtr> &allocated_regs() const {
    return allocated_regs_;
  }

 protected:
  // allocate a new slot
  OprPtr AllocSlot() const { return slot_alloc_(); }
  // add allocated register to set
  void MarkAsAllocated(const OprPtr &reg) {
    assert(reg->IsReg());
    allocated_regs_.insert(reg);
  }

 private:
  std::function<OprPtr()> slot_alloc_;
  std::unordered_set<OprPtr> allocated_regs_;
};

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASSES_REGALLOC_H_
