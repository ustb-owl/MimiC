#include "back/asm/arch/aarch32/instdef.h"

#include <cassert>

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
  "nop",
  "ldr", "str", "push", "pop",
  "add", "sub", "mul", "mls", "sdiv", "udiv",
  "cmp", "beq", "b", "bl",
  "",
  "mov", "movw",
  "and", "orr", "eor",
  "lsl", "lsr", "asr",
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

std::ostream &operator<<(std::ostream &os, const OprPtrList &oprs) {
  for (std::size_t i = 0; i < oprs().size(); ++i) {
    if (i) os << ", ";
    oprs[i]->Dump(os);
  }
  return os;
}

}  // namespace

void AArch32Reg::Dump(std::ostream &os) const {
  os << name_;
}

void AArch32Imm::Dump(std::ostream &os) const {
  os << '#' << val_;
}

void AArch32Slot::Dump(std::ostream &os) const {
  os << "[sp, #" << offset_ * 4 << ']';
}

void AArch32Inst::Dump(std::ostream &os) const {
  using OpCode = AArch32Inst::OpCode;
  if (opcode_ == OpCode::LABEL) {
    os << oprs()[0] << ':';
  }
  else {
    os << '\t' << opcode_;
    switch (opcode_) {
      case OpCode::PUSH: case OpCode::POP: {
        os << "\t{" << oprs() << '}';
        break;
      }
      default: {
        if (!dest() && !oprs().empty()) {
          os << '\t' << oprs();
        }
        else if (dest()) {
          os << '\t' << dest() << ", " << oprs();
        }
        break;
      }
    }
  }
  os << std::endl;
}
