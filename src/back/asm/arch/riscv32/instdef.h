#ifndef MIMIC_BACK_ASM_ARCH_RISCV32_INSTDEF_H_
#define MIMIC_BACK_ASM_ARCH_RISCV32_INSTDEF_H_

#include <string>
#include <cstdint>
#include <cassert>

#include "back/asm/mir/mir.h"

namespace mimic::back::asmgen::riscv32 {

// riscv32 register
class RISCV32Reg : public OperandBase {
 public:
  enum class RegName {
    // hard-wired zero
    Zero,
    // return address
    RA,
    // stack pointer
    SP,
    // global pointer & thread pointer
    X3, X4,
    // temporaries
    T0, T1, T2,
    // frame pointer
    FP,
    // saved register
    S1,
    // arguments/return values
    A0, A1,
    // arguments
    A2, A3, A4, A5, A6, A7,
    // saved registers
    S2, S3, S4, S5, S6, S7, S8, S9, S10, S11,
    // temporaries
    T3, T4, T5, T6,
  };

  RISCV32Reg(RegName name) : name_(name) {}

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

// riscv32 immediate number
class RISCV32Imm : public OperandBase {
 public:
  RISCV32Imm(std::int32_t val) : val_(val) {}

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

// riscv32 integer (used in directives)
class RISCV32Int : public OperandBase {
 public:
  RISCV32Int(std::int32_t val) : val_(val) {}

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

// riscv32 string (used in directives)
class RISCV32Str : public OperandBase {
 public:
  RISCV32Str(const std::string &str) : str_(str) {}

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

// riscv32 slot
class RISCV32Slot : public OperandBase {
 public:
  RISCV32Slot(const OprPtr &base, std::int32_t offset)
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

// riscv32 instruction
class RISCV32Inst : public InstBase {
 public:
  enum class OpCode {
    // memory accessing
    LW, LB, LBU, SW, SB,
    // arithmetic
    ADDI, SLTI, SLTIU, ADD, SUB, SLT, SLTU,
    MUL, DIV, DIVU, REM, REMU,
    NEG, SEQZ, SNEZ,
    // branch/jump
    CALL, RET, J,
    BEQ, BNE, BLT, BLE, BGT, BGE,
    BLTU, BLEU, BGTU, BGEU, BEQZ,
    // data processing
    LA, LI, MV,
    // logical
    XORI, ORI, ANDI, XOR, OR, AND,
    NOT,
    // shifting
    SLLI, SRLI, SRAI, SLL, SRL, SRA,
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

  // add/sub/beq/br...
  RISCV32Inst(OpCode opcode, const OprPtr &dest, const OprPtr &opr1,
              const OprPtr &opr2)
      : opcode_(opcode) {
    if ((static_cast<int>(opcode_) >= static_cast<int>(OpCode::BEQ) &&
         static_cast<int>(opcode_) <= static_cast<int>(OpCode::BGEU)) ||
        opcode_ == OpCode::BR) {
      set_dest(nullptr);
      AddOpr(dest);
    }
    else {
      set_dest(dest);
    }
    AddOpr(opr1);
    AddOpr(opr2);
  }
  // lw/beqz/mv/...
  RISCV32Inst(OpCode opcode, const OprPtr &dest, const OprPtr &opr)
      : opcode_(opcode) {
    if (opcode_ == OpCode::SW || opcode_ == OpCode::SB ||
        opcode_ == OpCode::BEQZ) {
      // SW/SB/BEQZ does not have destination register
      set_dest(nullptr);
      AddOpr(dest);
    }
    else {
      set_dest(dest);
    }
    AddOpr(opr);
  }
  // call/j/label/...
  RISCV32Inst(OpCode opcode, const OprPtr &opr)
      : opcode_(opcode) {
    set_dest(nullptr);
    AddOpr(opr);
  }
  // ret/...
  RISCV32Inst(OpCode opcode) : opcode_(opcode) {
    set_dest(nullptr);
  }

  bool IsMove() const override { return opcode_ == OpCode::MV; }
  bool IsLabel() const override { return opcode_ == OpCode::LABEL; }
  bool IsCall() const override { return opcode_ == OpCode::CALL; }
  void Dump(std::ostream &os) const override;

  // setters
  void set_opcode(OpCode opcode) { opcode_ = opcode; }

  // getters
  OpCode opcode() const { return opcode_; }

 private:
  OpCode opcode_;
};

}  // namespace mimic::back::asmgen::riscv32

#endif  // MIMIC_BACK_ASM_ARCH_RISCV32_INSTDEF_H_
