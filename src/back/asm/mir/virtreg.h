#ifndef BACK_ASM_MIR_VIRTREG_H_
#define BACK_ASM_MIR_VIRTREG_H_

#include "back/asm/mir/mir.h"

namespace mimic::back::asmgen {

// virtual register
class VirtRegOperand : public OperandBase {
 public:
  VirtRegOperand(std::uint32_t id) : id_(id) {}

  bool IsReg() const override { return true; }
  bool IsVirtual() const override { return true; }
  bool IsImm() const override { return false; }
  bool IsLabel() const override { return false; }
  bool IsSlot() const override { return false; }
  void Dump(std::ostream &os) const override {
    os << "vreg[" << id_ << ']';
  }

  // setters
  void set_alloc_to(const OprPtr &alloc_to) { alloc_to_ = alloc_to; }

  // getters
  std::uint32_t id() const { return id_; }
  const OprPtr &alloc_to() const { return alloc_to_; }

 private:
  std::uint32_t id_;
  OprPtr alloc_to_;
};

// virtual register factory
class VirtRegFactory {
 public:
  VirtRegFactory() : next_id_(0) {}

  // get a new virtual register (32-bit)
  OprPtr GetReg() {
    return std::make_shared<VirtRegOperand>(next_id_++);
  }

 private:
  std::uint32_t next_id_;
};

}  // namespace mimic::back::asmgen

#endif  // BACK_ASM_MIR_VIRTREG_H_
