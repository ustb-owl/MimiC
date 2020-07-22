#ifndef MIMIC_BACK_AARCH32_ASMGEN_H
#define MIMIC_BACK_AARCH32_ASMGEN_H

#include <list>
#include <ostream>

#include "back/aarch32/armv7asm.h"

namespace mimic::back::aarch32 {

class ARMv7AsmGen {
 public:
  ARMv7AsmGen() { Reset(); }

  // reset internal state
  void Reset() {
    asms_.clear();
    is_reordered_ = false;
  }

  // push a new assembly
  template <typename... Args>
  void PushAsm(Args &&... args) {
    asms_.emplace_back(std::forward<Args>(args)...);
  }

  // push 'nop' pseudo instruction
  void PushNop();
  // push a label
  void PushLabel(std::string_view label);
  // push 'mov' pseudo instruction
  void PushMove(ARMv7Reg dest, ARMv7Reg src);
  // push 'ldr' pseudo instruction
  void PushLoadImm(ARMv7Reg dest, std::uint32_t imm);
  // push 'ldr?' pseudo instruction
  void PushLoadImm(ARMv7Reg dest, std::string_view imm_str);
  // push a new branch instruction
  void PushBranch(ARMv7Reg cond, std::string_view label);
  // push a new jump instruction
  void PushJump(std::string_view label);
  // push a new jump instruction
  void PushJump(ARMv7Reg dest);
  // dump assembly code to stream
  void Dump(std::ostream &os, std::string_view indent);
  void PopBack() {
    asms_.pop_back();
  }

 private:
  // reorder all jumps (make delay slots)
  void ReorderJump();

  std::list<ARMv7Asm> asms_;
  bool is_reordered_;
};

}  // namespace mimic::back::aarch32

#endif  // MIMIC_BACK_AARCH32_ASMGEN_H