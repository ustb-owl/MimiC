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
  "cmp", "beq", "b", "bl", "bx",
  "mov", "movw", "movt", "mvn",
  "moveq", "movwne",
  "movwlo", "movwlt", "movwls", "movwle",
  "movwhi", "movwgt", "movwhs", "movwge",
  "and", "orr", "eor",
  "lsl", "lsr", "asr",
  "clz",
  "sxtb", "uxtb",
  "",
  ".zero", ".asciz", ".long", ".byte",
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
  os << '[' << (based_on_sp_ ? "sp" : "r11") << ", #" << offset_ << ']';
}

void AArch32Inst::Dump(std::ostream &os) const {
  using OpCode = AArch32Inst::OpCode;
  if (opcode_ == OpCode::LABEL) {
    os << oprs()[0].value() << ':';
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
  os << std::endl;
}
