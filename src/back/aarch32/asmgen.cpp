#include "back/aarch32/asmgen.h"

using namespace mimic::back::aarch32;

namespace {

using Asm = ARMv7Asm;
using Opcode = ARMv7Opcode;
using Reg = ARMv7Reg;

const char *opcode_str[] = {
    "nop",
    "ldr", "str", "ldmea", "stmfd",
    "add", "sub", "mul",
    "__aeabi_idiv", "__aeabi_idivmod", "__aeabi_uidiv", "__aeabi_uidivmod",
    "b", "bl", "bx", "bne", "beq",
    "cmp", "cmpne", "",
  
    "mov", "mvn", "movne", "moveq",
    "movgt", "movge", "movlt", "movle",
    "movhi", "movhs", "movlo", "movls",

    "and", "orr", "eor",
    "lsl", "lsr", "asr",
    
    "",
};

const char *reg_str[] = {
    "r0", "r1", "r2", "r3",
    "r4", "r5", "r6", "r7",
    "r8", "r9", 
    "r10",
    "fp", "ip", "sp", "lr", "pc",
};

// check if instruction is pseudo instruction
inline bool IsPseudo(const Asm &tm) {
  return static_cast<int>(tm.opcode) >= static_cast<int>(Opcode::NOP);
}

// check if instruction is just a label
inline bool IsLabel(const Asm &tm) {
  return tm.opcode == Opcode::LABEL;
}

// check if instruction is jump or branch
inline bool IsJump(const Asm &tm) {
  const auto &op = tm.opcode;
  return op == Opcode::BEQ || op == Opcode::BNE || op == Opcode::B ||
         op == Opcode::BL || op == Opcode::BX;
}

// check if instruction is store
inline bool IsStore(const Asm &tm) {
  return tm.opcode == Opcode::STR || tm.opcode == Opcode::STMFD;
}

// check if current instruction has dest
// TODO: fix: no dest
inline bool HasDest(const Asm &tm) {
  return !IsPseudo(tm) && !IsJump(tm) && !IsStore(tm);
}

// check if two registers are related
// inline bool IsRegRelated(Reg r1, Reg r2) {
//   return r1 == r2;
// }

// check if registers and asm are related
// inline bool IsRelated(const Asm &tm, Reg op, Reg dest) {
//   switch (tm.opcode) {
//     case Opcode::ADD: case Opcode::SUB: case Opcode::MUL:
//     case Opcode::DIV: case Opcode::MOD: {
//       return IsRegRelated(tm.dest, op) || IsRegRelated(dest, tm.opr1) ||
//              IsRegRelated(dest, tm.opr2);
//     }
//     case Opcode::BEQ: case Opcode::BNE: {
//       return IsRegRelated(dest, tm.opr1) || IsRegRelated(dest, tm.opr2);
//     }
//     case Opcode::LDR: case Opcode::LDMEA: {
//       return IsRegRelated(tm.dest, op) || IsRegRelated(dest, tm.opr1);
//     }
//     case Opcode::STR: case Opcode::STMFD: {
//       return IsRegRelated(dest, tm.dest) || IsRegRelated(dest, tm.opr1);
//     }
//     default:;
//   }
//   return false;
// }

// check if registers and asm are related
// TODO: fix: no Reg::Zero
inline bool IsRelated(const Asm &tm, Reg op) {
  // return IsRelated(tm, op, Reg::Zero);
  return false;
}

// check if instruction and branch are related
inline bool IsBranchRelated(const Asm &tm, const Asm &branch) {
  return IsRelated(tm, branch.dest) || IsRelated(tm, branch.opr1);
}

// dump content of asm to stream
void DumpAsm(std::ostream &os, const Asm &tm) {
  switch (tm.opcode) {
    case Opcode::ADD: case Opcode::SUB: case Opcode::MUL:
    case Opcode::sDIV: case Opcode::sMOD:
    case Opcode::uDIV: case Opcode::uMOD: 
    case Opcode::AND: case Opcode::ORR: case Opcode::EOR:
    case Opcode::LSL: case Opcode::LSR: case Opcode::ASR:{
      os << opcode_str[static_cast<int>(tm.opcode)] << '\t';
      os << reg_str[static_cast<int>(tm.dest)] << ", ";
      os << reg_str[static_cast<int>(tm.opr1)] << ", ";
      if (tm.is_imm) {
        os << "#" << tm.imm;
      } else {
        os << reg_str[static_cast<int>(tm.opr2)];
      }
      break;
    }
    case Opcode::MOV: case Opcode::MVN:
    case Opcode::MOVNE: case Opcode::MOVEQ:
    case Opcode::MOVGT: case Opcode::MOVGE:
    case Opcode::MOVLT: case Opcode::MOVLE:
    case Opcode::MOVHI: case Opcode::MOVHS:
    case Opcode::MOVLO: case Opcode::MOVLS:{
      os << opcode_str[static_cast<int>(tm.opcode)] << '\t';
      os << reg_str[static_cast<int>(tm.dest)] << ", ";
      if (tm.is_imm) {
        os << "#" << tm.imm;
      } else {
        os << reg_str[static_cast<int>(tm.opr1)];
      }
      break;
    }
    case Opcode::LABEL: {
      os << tm.imm_str << ':';
      break;
    }
    case Opcode::INST: {
      os << tm.imm_str;
      break;
    }
    case Opcode::LDR: case Opcode::STR: {
      if (static_cast<int>(tm.opr1) != static_cast<int>(Reg::fp)) {
        if (tm.opcode == Opcode::STR) {
          os << opcode_str[static_cast<int>(Opcode::MOV)] << '\t';
          os << reg_str[static_cast<int>(tm.opr1)] << ", ";
          os << reg_str[static_cast<int>(tm.dest)];
        }
        else {
          os << opcode_str[static_cast<int>(Opcode::MOV)] << '\t';
          os << reg_str[static_cast<int>(tm.dest)] << ", ";
          os << reg_str[static_cast<int>(tm.opr1)];
        }
      }
      else {
        os << opcode_str[static_cast<int>(tm.opcode)] << '\t';
        os << reg_str[static_cast<int>(tm.dest)] << ", [";
        os << reg_str[static_cast<int>(tm.opr1)];
        if (tm.imm) os << ", #" << tm.imm;
        os << "]";
      }
      break;
    }
    case Opcode::CMP: case Opcode::CMPNE: {
      os << opcode_str[static_cast<int>(tm.opcode)] << '\t';
      os << reg_str[static_cast<int>(tm.dest)] << ", ";
      if (tm.is_imm) {
        os << "#" << tm.imm;
      } else {
        os << reg_str[static_cast<int>(tm.opr1)];
      }
      break;
    }
    case Opcode::BEQ: case Opcode::BNE: case Opcode::B: {
      os << opcode_str[static_cast<int>(tm.opcode)] << '\t';
      if (!tm.imm_str.empty()) {
        os << tm.imm_str;
      }
      else {
        os << tm.index;
      }
      break;
    }
    default: {
      // if (!tm.imm_str.empty()) {
      //   os << tm.imm_str;
      // }
    };
  }
}

}  // namespace

void ARMv7AsmGen::ReorderJump() {
  // check if is already reordered
  if (is_reordered_) return;
  is_reordered_ = true;
  // record position and last elements
  std::size_t pos = 0;
  auto last = asms_.end(), last2 = asms_.end();
  // traverse all instructions
  for (auto it = asms_.begin(); it != asms_.end(); ++it, ++pos) {
    switch (it->opcode) {
      case Opcode::BEQ: case Opcode::BNE: {
        // check if is related
        if (!pos ||
            (pos && (IsBranchRelated(*last, *it) || IsLabel(*last))) ||
            (pos > 1 && IsJump(*last2))) {
          // insert NOP after branch
          last = it;
          it = asms_.insert(++it, {Opcode::NOP});
        }
        else {
          // just swap
          std::swap(*last, *it);
        }
        break;
      }
      default:;
    }
    // update iterators
    last2 = last;
    last = it;
  }
}

void ARMv7AsmGen::PushNop() {
  PushAsm(Opcode::NOP);
}

void ARMv7AsmGen::PushLabel(std::string_view label) {
  // remove redundant jumps
  while (asms_.back().opcode == Opcode::B &&
         asms_.back().imm_str == label) {
    asms_.pop_back();
  }
  PushAsm(Opcode::LABEL, label);
}

void ARMv7AsmGen::PushMove(Reg dest, Reg src) {
  // do not generate redundant move
  if (dest == src) return;
  // try to modify the dest of last instruction
  if (HasDest(asms_.back()) && asms_.back().dest == src) {
    asms_.back().dest = dest;
  }
  else {
    PushAsm(Opcode::MOV, dest, src);
  }
}

void ARMv7AsmGen::PushLoadImm(Reg dest, std::uint32_t imm) {
  if ((!(imm & 0xffff0000) && !(imm & 0x8000)) ||
      ((imm & 0xffff0000) == 0xffff0000 && (imm & 0x8000))) {
    PushAsm(Opcode::MOV, dest, imm);
  }
  // else if (!(imm & 0x8000)) {
  //   PushAsm(Opcode::LUI, dest, imm >> 16);
  //   PushAsm(Opcode::ADD, dest, dest, imm & 0xffff);
  // }
  // else {
  //   PushAsm(Opcode::LUI, dest, (imm >> 16) + 1);
  //   PushAsm(Opcode::ADD, dest, dest, imm & 0xffff);
  // }
}

void ARMv7AsmGen::PushLoadImm(Reg dest, std::string_view imm_str) {
  using namespace std::string_literals;
  PushAsm(Opcode::LDR, dest, imm_str.data());
}

void ARMv7AsmGen::PushBranch(Reg cond, std::string_view label) {
}

void ARMv7AsmGen::PushJump(std::string_view label) {
  PushAsm(Opcode::B, label);
}

void ARMv7AsmGen::PushJump(Reg dest) {
  PushAsm(Opcode::B, dest);
}

void ARMv7AsmGen::Dump(std::ostream &os, std::string_view indent) {
  // ReorderJump();
  for (const auto &i : asms_) {
    if (i.opcode != Opcode::LABEL) os << indent;
    DumpAsm(os, i);
    os << std::endl;
  }
}
