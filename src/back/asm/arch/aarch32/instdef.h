#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_INSTDEF_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_INSTDEF_H_

#include <initializer_list>
#include <cstdint>

#include "back/asm/mir/mir.h"

namespace mimic::back::asmgen::aarch32 {

// aarch32 register
class AArch32Reg : public OperandBase {
 public:
  enum class RegName {
    // argument/result/scratch
    R0, R1,
    // argument/scratch
    R2, R3,
    // variable
    R4, R5, R6, R7, R8,
    // platform
    R9,
    // variable
    R10,
    // frame pointer/variable
    R11,
    // intra-procedure-call scratch
    R12,
    // stack pointer
    SP,
    // link register
    LR,
    // program counter
    PC,
  };

  AArch32Reg(RegName name) {}

  bool IsReg() const override { return true; }
  bool IsVirtual() const override { return false; }
  bool IsImm() const override { return false; }
  bool IsLabel() const override { return false; }
  bool IsSlot() const override { return false; }

  void Dump(std::ostream &os) const override;

  // getters
  RegName name() const { return name_; }

 private:
  RegName name_;
};

// aarch32 immediate number
class AArch32Imm : public OperandBase {
 public:
  AArch32Imm(std::int32_t val) : val_(val) {}

  bool IsReg() const override { return false; }
  bool IsVirtual() const override { return false; }
  bool IsImm() const override { return true; }
  bool IsLabel() const override { return false; }
  bool IsSlot() const override { return false; }

  void Dump(std::ostream &os) const override;

  // getters
  std::int32_t val() const { return val_; }

 private:
  std::int32_t val_;
};

// aarch32 stack slot
class AArch32Slot : public OperandBase {
 public:
  AArch32Slot(std::int32_t offset) : offset_(offset) {}

  bool IsReg() const override { return false; }
  bool IsVirtual() const override { return false; }
  bool IsImm() const override { return false; }
  bool IsLabel() const override { return false; }
  bool IsSlot() const override { return true; }

  void Dump(std::ostream &os) const override;

  // getters
  std::int32_t offset() const { return offset_; }

 private:
  std::int32_t offset_;
};

// aarch32 instruction
class AArch32Inst : public InstBase {
 public:
  enum class OpCode {
    // no operation
    NOP,
    // memory accessing
    LDR, STR, PUSH, POP,
    // arithmetic
    ADD, SUB, MUL, MLS, SDIV, UDIV,
    // comparison/branch/jump
    CMP, BEQ, B, BL,
    // just a label definition
    LABEL,
    // data moving
    MOV, MOVW,
    // logical
    AND, ORR, EOR,
    // shifting
    LSL, LSR, ASR,
  };

  // push/pop/...
  AArch32Inst(OpCode opcode, std::initializer_list<OprPtr> oprs)
      : opcode_(opcode) {
    set_dest(nullptr);
    for (const auto &i : oprs) AddOpr(i);
  }
  // mls/...
  AArch32Inst(OpCode opcode, const OprPtr &dest, const OprPtr &opr1,
              const OprPtr &opr2, const OprPtr &opr3)
      : opcode_(opcode) {
    set_dest(dest);
    AddOpr(opr1);
    AddOpr(opr2);
    AddOpr(opr3);
  }
  // add/sub/...
  AArch32Inst(OpCode opcode, const OprPtr &dest, const OprPtr &opr1,
              const OprPtr &opr2)
      : opcode_(opcode) {
    set_dest(dest);
    AddOpr(opr1);
    AddOpr(opr2);
  }
  // ldr/mov/...
  AArch32Inst(OpCode opcode, const OprPtr &dest, const OprPtr &opr)
      : opcode_(opcode) {
    set_dest(dest);
    AddOpr(opr);
  }
  // beq/label/...
  AArch32Inst(OpCode opcode, const OprPtr &opr)
      : opcode_(opcode) {
    set_dest(nullptr);
    AddOpr(opr);
  }
  // nop/...
  AArch32Inst(OpCode opcode) : opcode_(opcode) { set_dest(nullptr); }

  bool IsMove() const override {
    return opcode_ == OpCode::MOV || opcode_ == OpCode::MOVW;
  }
  void Dump(std::ostream &os) const override;

  // getters
  OpCode opcode() const { return opcode_; }

 private:
  OpCode opcode_;
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_INSTDEF_H_
