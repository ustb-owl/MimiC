#include "back/asm/arch/aarch32/instdef.h"

#include <cassert>

#include "utils/strprint.h"

using namespace mimic::back::asmgen;
using namespace mimic::back::asmgen::aarch32;

namespace {

const char *kRegNames[] = {
  "r0", "r1", "r2", "r3",
  "r4", "r5", "r6", "r7",
  "r8", "r9", "r10", "r11",
  "r12", "sp", "lr", "pc",
};

const char *kOpCodes[] = {
  "ldr", "ldrb", "str", "strb", "push", "pop",
  "add", "sub", "subs", "rsb", "mul", "mls", "sdiv", "udiv",
  "cmp", "b", "bl", "bx",
  "beq", "bne",
  "blo", "blt", "bls", "ble",
  "bhi", "bgt", "bhs", "bge",
  "mov", "movw", "movt", "mvn",
  "moveq", "movwne",
  "movwlo", "movwlt", "movwls", "movwle",
  "movwhi", "movwgt", "movwhs", "movwge",
  "and", "orr", "eor",
  "lsl", "lsr", "asr",
  "clz",
  "sxtb", "uxtb",
  "",
  "lea", "br",
  "seteq", "setne",
  "setult", "setslt", "setule", "setsle",
  "setugt", "setsgt", "setuge", "setsge",
  ".zero", ".asciz", ".long", ".byte",
};

const char *kShiftOp[] = {
  "", "lsl", "lsr", "asr", "ror",
};

std::ostream &operator<<(std::ostream &os, AArch32Reg::RegName name) {
  os << kRegNames[static_cast<int>(name)];
  return os;
}

std::ostream &operator<<(std::ostream &os, AArch32Inst::OpCode opcode) {
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

void DumpMemOpr(std::ostream &os, const OprPtr &opr) {
  if (opr->IsLabel()) {
    os << '=' << opr;
  }
  else if (opr->IsReg()) {
    os << '[' << opr << ']';
  }
  else {
    os << opr;
  }
}

}  // namespace

void AArch32Reg::Dump(std::ostream &os) const {
  os << name_;
}

void AArch32Imm::Dump(std::ostream &os) const {
  os << '#' << val_;
}

void AArch32Int::Dump(std::ostream &os) const {
  os << val_;
}

void AArch32Str::Dump(std::ostream &os) const {
  os << '"';
  utils::DumpStr(os, str_);
  os << '"';
}

void AArch32Slot::Dump(std::ostream &os) const {
  os << '[' << base_ << ", #" << offset_ << ']';
}

void AArch32Inst::Dump(std::ostream &os) const {
  using OpCode = AArch32Inst::OpCode;
  if (opcode_ == OpCode::LABEL) {
    os << oprs()[0].value() << ':';
  }
  else if (opcode_ == OpCode::SDIV || opcode_ == OpCode::UDIV) {
    // NOTE: a dirty hack for ARMv7-A assembler on ARMv8 architecture
    assert(dest() && oprs().size() == 2);
    os << '\t';
    if (dest()->IsReg() && !dest()->IsVirtual() &&
        oprs()[0].value()->IsReg() && !oprs()[0].value()->IsVirtual() &&
        oprs()[1].value()->IsReg() && !oprs()[1].value()->IsVirtual()) {
      // dump binary
      auto rd = static_cast<AArch32Reg *>(dest().get());
      auto rn = static_cast<AArch32Reg *>(oprs()[0].value().get());
      auto rm = static_cast<AArch32Reg *>(oprs()[1].value().get());
      std::uint32_t w = opcode_ == OpCode::SDIV ? 0xe710f010 : 0xe730f010;
      auto d = static_cast<std::uint32_t>(rd->name());
      auto n = static_cast<std::uint32_t>(rn->name());
      auto m = static_cast<std::uint32_t>(rm->name());
      w = w | (d << 16) | (m << 8) | (n << 0);
      os << ".word\t0x" << std::hex << w << std::dec;
    }
    else {
      // just dump
      os << opcode_ << '\t' << dest() << ", " << oprs();
    }
  }
  else {
    os << '\t' << opcode_ << '\t';
    switch (opcode_) {
      case OpCode::PUSH: case OpCode::POP: {
        os << '{' << oprs() << '}';
        break;
      }
      case OpCode::LDR: case OpCode::LDRB: {
        os << dest() << ", ";
        DumpMemOpr(os, oprs()[0].value());
        break;
      }
      case OpCode::STR: case OpCode::STRB: {
        os << oprs()[0].value() << ", ";
        DumpMemOpr(os, oprs()[1].value());
        break;
      }
      case OpCode::MOVW: {
        os << dest() << ", ";
        if (oprs()[0].value()->IsLabel()) {
          os << "#:lower16:" << oprs()[0].value();
        }
        else {
          os << oprs()[0].value();
        }
        break;
      }
      case OpCode::MOVT: {
        os << dest() << ", ";
        if (oprs()[0].value()->IsLabel()) {
          os << "#:upper16:" << oprs()[0].value();
        }
        else {
          os << oprs()[0].value();
        }
        break;
      }
      default: {
        if (!dest() && !oprs().empty()) {
          os << oprs();
        }
        else if (dest()) {
          os << dest() << ", " << oprs();
        }
        break;
      }
    }
  }
  // dump shifted operand suffix
  if (shift_op_ != ShiftOp::NOP) {
    os << ", " << kShiftOp[static_cast<int>(shift_op_)];
    os << " #" << static_cast<unsigned>(shift_amt_);
  }
  os << std::endl;
}
