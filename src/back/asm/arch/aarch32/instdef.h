#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_INSTDEF_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_INSTDEF_H_

#include <string>
#include <initializer_list>
#include <cstdint>
#include <cassert>

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

  AArch32Reg(RegName name) : name_(name) {}

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

// aarch32 integer (used in directives)
class AArch32Int : public OperandBase {
 public:
  AArch32Int(std::int32_t val) : val_(val) {}

  bool IsReg() const override { return false; }
  bool IsVirtual() const override { return false; }
  bool IsImm() const override { return false; }
  bool IsLabel() const override { return false; }
  bool IsSlot() const override { return false; }

  void Dump(std::ostream &os) const override;

  // getters
  std::int32_t val() const { return val_; }

 private:
  std::int32_t val_;
};

// aarch32 string (used in directives)
class AArch32Str : public OperandBase {
 public:
  AArch32Str(const std::string &str) : str_(str) {}

  bool IsReg() const override { return false; }
  bool IsVirtual() const override { return false; }
  bool IsImm() const override { return false; }
  bool IsLabel() const override { return false; }
  bool IsSlot() const override { return false; }

  void Dump(std::ostream &os) const override;

  // getters
  const std::string &str() const { return str_; }

 private:
  std::string str_;
};

// aarch32 slot
class AArch32Slot : public OperandBase {
 public:
  AArch32Slot(const OprPtr &base, std::int32_t offset)
      : base_(base), offset_(offset) {
    assert(base->IsReg());
  }

  bool IsReg() const override { return false; }
  bool IsVirtual() const override { return false; }
  bool IsImm() const override { return false; }
  bool IsLabel() const override { return false; }
  bool IsSlot() const override { return true; }

  void Dump(std::ostream &os) const override;

  // getters
  const OprPtr &base() const { return base_; }
  std::int32_t offset() const { return offset_; }

 private:
  OprPtr base_;
  std::int32_t offset_;
};

// aarch32 instruction
class AArch32Inst : public InstBase {
 public:
  enum class OpCode {
    // memory accessing
    LDR, LDRB, STR, STRB, PUSH, POP,
    // arithmetic
    ADD, SUB, SUBS, RSB,
    MUL, MLS, SMMUL, UMULL, SDIV, UDIV,
    // comparison/branch/jump
    CMP, B, BL, BX,
    BEQ, BNE,
    BLO, BLT, BLS, BLE,
    BHI, BGT, BHS, BGE,
    // data moving
    MOV, MOVW, MOVT, MVN,
    MOVEQ, MOVWNE,
    MOVWLO, MOVWLT, MOVWLS, MOVWLE,
    MOVWHI, MOVWGT, MOVWHS, MOVWGE,
    // logical
    AND, ORR, EOR,
    // shifting
    LSL, LSR, ASR,
    // bit manipulating
    CLZ,
    // extending
    SXTB, UXTB,
    // just a label definition
    LABEL,
    // pseudo instructions
    LEA, BR,
    SETEQ, SETNE,
    SETULT, SETSLT, SETULE, SETSLE,
    SETUGT, SETSGT, SETUGE, SETSGE,
    // assembler directives
    ZERO, ASCIZ, LONG, BYTE,
  };

  // shift opcode for shifted operands
  enum class ShiftOp {
    NOP, LSL, LSR, ASR, ROR,
  };

  // push/pop/...
  AArch32Inst(OpCode opcode, std::initializer_list<OprPtr> oprs)
      : opcode_(opcode), shift_op_(ShiftOp::NOP) {
    set_dest(nullptr);
    for (const auto &i : oprs) AddOpr(i);
  }
  // mls/...
  AArch32Inst(OpCode opcode, const OprPtr &dest, const OprPtr &opr1,
              const OprPtr &opr2, const OprPtr &opr3)
      : opcode_(opcode), shift_op_(ShiftOp::NOP) {
    set_dest(dest);
    AddOpr(opr1);
    AddOpr(opr2);
    AddOpr(opr3);
  }
  // add/sub/...
  AArch32Inst(OpCode opcode, const OprPtr &dest, const OprPtr &opr1,
              const OprPtr &opr2)
      : opcode_(opcode), shift_op_(ShiftOp::NOP) {
    if (opcode == OpCode::BR) {
      set_dest(nullptr);
      AddOpr(dest);
    }
    else {
      set_dest(dest);
    }
    AddOpr(opr1);
    AddOpr(opr2);
  }
  // ldr/mov/cmp/...
  AArch32Inst(OpCode opcode, const OprPtr &dest, const OprPtr &opr)
      : opcode_(opcode), shift_op_(ShiftOp::NOP) {
    if (opcode_ == OpCode::CMP || opcode_ == OpCode::STR ||
        opcode_ == OpCode::STRB) {
      // CMP/STR/STRB does not have destination register
      set_dest(nullptr);
      AddOpr(dest);
    }
    else {
      set_dest(dest);
    }
    AddOpr(opr);
  }
  // beq/label/...
  AArch32Inst(OpCode opcode, const OprPtr &opr)
      : opcode_(opcode), shift_op_(ShiftOp::NOP) {
    set_dest(nullptr);
    AddOpr(opr);
  }
  // nop/...
  AArch32Inst(OpCode opcode) : opcode_(opcode), shift_op_(ShiftOp::NOP) {
    set_dest(nullptr);
  }

  bool IsMove() const override { return opcode_ == OpCode::MOV; }
  bool IsLabel() const override { return opcode_ == OpCode::LABEL; }
  bool IsCall() const override { return opcode_ == OpCode::BL; }
  void Dump(std::ostream &os) const override;

  // setters
  void set_shift_op_amt(ShiftOp op, std::uint8_t amt) {
    shift_op_ = op;
    shift_amt_ = amt;
  }

  // getters
  OpCode opcode() const { return opcode_; }
  ShiftOp shift_op() const { return shift_op_; }
  std::uint8_t shift_amt() const { return shift_amt_; }

 private:
  OpCode opcode_;
  ShiftOp shift_op_;
  std::uint8_t shift_amt_;
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_INSTDEF_H_
