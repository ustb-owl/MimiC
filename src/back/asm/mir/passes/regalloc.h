#ifndef MIMIC_BACK_ASM_MIR_PASSES_REGALLOC_H_
#define MIMIC_BACK_ASM_MIR_PASSES_REGALLOC_H_

#include <functional>
#include <unordered_set>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/mir/virtreg.h"

namespace mimic::back::asmgen {

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

  // add avaliable architecture register
  virtual void AddAvaliableReg(const OprPtr &reg) = 0;

  // setters
  // specify stack slot allocator
  void set_allocator(SlotAllocator allocator) { allocator_ = allocator; }

 protected:
  // specify 'alloc_to' property of the specific
  void AllocateVRegTo(const OprPtr &vreg, const OprPtr &dest) {
    assert(vreg->IsVirtual() &&
           ((dest->IsReg() && !dest->IsVirtual()) || dest->IsSlot()));
    static_cast<VirtRegOperand *>(vreg.get())->set_alloc_to(dest);
  }

  // getters
  const SlotAllocator &allocator() const { return allocator_; }

 private:
  SlotAllocator allocator_;
};

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASSES_REGALLOC_H_
