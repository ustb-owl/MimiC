#ifndef BACK_ASM_MIR_VIRTREG_H_
#define BACK_ASM_MIR_VIRTREG_H_

#include "back/asm/mir/mir.h"

namespace mimic::back::asmgen {

// virtual register
class VirtRegOperand : public OperandBase {
 public:
  VirtRegOperand(std::uint32_t id) : id_(id) {}

  bool IsReg() const { return true; }
  bool IsVirtual() const { return true; }
  bool IsImm() const { return false; }
  bool IsLabel() const { return false; }
  bool IsSlot() const { return false; }
  bool IsMem() const { return false; }
  void Dump(std::ostream &os) const { os << "vreg[" << id_ << std::endl; }

 private:
  std::uint32_t id_;
};

// virtual register factory
class VirtRegFactory {
 public:
  VirtRegFactory() : next_id_(0) {}

  // get a new virtual register
  OprPtr GetReg() { return std::make_shared<VirtRegOperand>(next_id_++); }

 private:
  std::uint32_t next_id_;
};

}  // namespace mimic::back::asmgen

#endif  // BACK_ASM_MIR_VIRTREG_H_
