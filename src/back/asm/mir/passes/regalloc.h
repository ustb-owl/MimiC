#ifndef MIMIC_BACK_ASM_MIR_PASSES_REGALLOC_H_
#define MIMIC_BACK_ASM_MIR_PASSES_REGALLOC_H_

#include <functional>
#include <unordered_set>
#include <cassert>

#include "back/asm/mir/pass.h"

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

class RegAllocatorBase : public PassInterface {
 public:
  virtual ~RegAllocatorBase() = default;

  // add avaliable architecture register
  virtual void AddAvaliableReg(const OprPtr &reg) = 0;

  // setters
  // specify stack slot allocator
  void set_allocator(SlotAllocator allocator) { allocator_ = allocator; }

 protected:
  // getters
  const SlotAllocator &allocator() const { return allocator_; }

 private:
  SlotAllocator allocator_;
};

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASSES_REGALLOC_H_
