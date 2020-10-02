#include "back/asm/arch/riscv32/instdef.h"

#include "utils/strprint.h"

using namespace mimic::back::asmgen;
using namespace mimic::back::asmgen::riscv32;

namespace {

const char *kRegNames[] = {
  "zero", "ra", "sp", "x3", "x4", "t0", "t1", "t2", "fp", "s1",
  "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
  "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
  "t3", "t4", "t5", "t6",
};

const char *kOpCodes[] = {
  "lw", "lb", "lbu", "sw", "sb",
  "addi", "slti", "sltiu", "add", "sub", "slt", "sltu",
  "mul", "div", "divu", "rem", "remu",
  "neg", "seqz", "snez",
  "call", "ret", "j",
  "beq", "bne", "blt", "ble", "bgt", "bge",
  "bltu", "bleu", "bgtu", "bgeu", "beqz",
  "la", "li", "mv",
  "xori", "ori", "andi", "xor", "or", "and",
  "not",
  "slli", "srli", "srai", "sll", "srl", "sra",
  "",
  "lea", "br",
  "seteq", "setne",
  "setult", "setslt", "setule", "setsle",
  "setugt", "setsgt", "setuge", "setsge",
  ".zero", ".asciz", ".long", ".byte",
};

std::ostream &operator<<(std::ostream &os, RISCV32Reg::RegName name) {
  os << kRegNames[static_cast<int>(name)];
  return os;
}

std::ostream &operator<<(std::ostream &os, RISCV32Inst::OpCode opcode) {
  os << kOpCodes[static_cast<int>(opcode)];
  return os;
}

std::ostream &operator<<(std::ostream &os, const OprPtr &opr) {
  opr->Dump(os);
  return os;
}

std::ostream &operator<<(std::ostream &os, const std::vector<Use> &oprs) {
  for (std::size_t i = 0; i < oprs.size(); ++i) {
    if (i) os << ", ";
    oprs[i].value()->Dump(os);
  }
  return os;
}

}  // namespace

void RISCV32Reg::Dump(std::ostream &os) const {
  os << name_;
}

void RISCV32Imm::Dump(std::ostream &os) const {
  os << val_;
}

void RISCV32Int::Dump(std::ostream &os) const {
  os << val_;
}

void RISCV32Str::Dump(std::ostream &os) const {
  os << '"';
  utils::DumpStr(os, str_);
  os << '"';
}

void RISCV32Slot::Dump(std::ostream &os) const {
  os << offset_ << '(' << base_ << ')';
}

void RISCV32Inst::Dump(std::ostream &os) const {
  using OpCode = RISCV32Inst::OpCode;
  if (opcode_ == OpCode::LABEL) {
    os << oprs()[0].value() << ':';
  }
  else {
    os << '\t' << opcode_ << '\t';
    if (!dest() && !oprs().empty()) {
      os << oprs();
    }
    else if (dest()) {
      os << dest() << ", " << oprs();
    }
  }
  os << std::endl;
}
