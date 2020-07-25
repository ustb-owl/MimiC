#ifndef BACK_ASM_MIR_VIRTREG_H_
#define BACK_ASM_MIR_VIRTREG_H_

#include "back/asm/mir/mir.h"

namespace mimic::back::asmgen {

// virtual register
class VirtRegOperand : public OperandBase {
 public:
  VirtRegOperand(std::uint32_t id, std::size_t size)
      : id_(id), size_(size) {}

  bool IsReg() const override { return true; }
  bool IsVirtual() const override { return true; }
  bool IsImm() const override { return false; }
  bool IsLabel() const override { return false; }
  bool IsSlot() const override { return false; }
  void Dump(std::ostream &os) const override {
    os << "vreg[" << id_ << ']';
  }

  // getters
  std::uint32_t id() const { return id_; }
  std::size_t size() const { return size_; }

 private:
  std::uint32_t id_;
  std::size_t size_;
};

// virtual register factory
class VirtRegFactory {
 public:
  VirtRegFactory() : next_id_(0) {}

  // get a new virtual register (32-bit)
  OprPtr GetReg() {
    return std::make_shared<VirtRegOperand>(next_id_++, 4);
  }

  // get a new virtual register (with specific size)
  OprPtr GetReg(std::size_t size) {
    return std::make_shared<VirtRegOperand>(next_id_++, size);
  }

 private:
  std::uint32_t next_id_;
};

}  // namespace mimic::back::asmgen

#endif  // BACK_ASM_MIR_VIRTREG_H_
