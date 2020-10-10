#ifndef MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_IMMCONV_H_
#define MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_IMMCONV_H_

#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/riscv32/instdef.h"
#include "back/asm/arch/riscv32/instgen.h"

namespace mimic::back::asmgen::riscv32 {

/*
  this pass will convert instructions to it's immediate version
  (if necessary and possible)
*/
class ImmConversionPass : public PassInterface {
 public:
  ImmConversionPass(RISCV32InstGen &gen) : gen_(gen) {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    for (const auto &i : insts) {
      auto inst = static_cast<RISCV32Inst *>(i.get());
      switch (inst->opcode()) {
        case OpCode::MV: {
          // convert all move instructions with immediate operand
          // to immediate loads
          if (inst->oprs().back().value()->IsImm()) {
            inst->set_opcode(OpCode::LI);
          }
          break;
        }
        case OpCode::ADD: case OpCode::SLT: case OpCode::SLTU:
        case OpCode::XOR: case OpCode::OR: case OpCode::AND:
        case OpCode::SLL: case OpCode::SRL: case OpCode::SRA: {
          if (IsValidImm(inst->oprs().back().value())) {
            inst->set_opcode(GetConvertedOpCode(inst->opcode()));
          }
          break;
        }
        case OpCode::SUB: {
          auto &last_opr = inst->oprs().back();
          if (last_opr.value()->IsImm()) {
            auto imm = static_cast<RISCV32Imm *>(last_opr.value().get());
            auto val = -imm->val();
            if (val >= -2048 && val <= 2047) {
              // update opcode & immediate
              inst->set_opcode(OpCode::ADDI);
              last_opr.set_value(gen_.GetImm(val));
            }
          }
          break;
        }
        default:;
      }
    }
  }

 private:
  using OpCode = RISCV32Inst::OpCode;

  bool IsValidImm(const OprPtr &opr) {
    if (!opr->IsImm()) return false;
    auto imm = static_cast<RISCV32Imm *>(opr.get())->val();
    return imm >= -2048 && imm <= 2047;
  }

  OpCode GetConvertedOpCode(OpCode opcode) {
    switch (opcode) {
      case OpCode::ADD: return OpCode::ADDI;
      case OpCode::SLT: return OpCode::SLTI;
      case OpCode::SLTU: return OpCode::SLTIU;
      case OpCode::XOR: return OpCode::XORI;
      case OpCode::OR: return OpCode::ORI;
      case OpCode::AND: return OpCode::ANDI;
      case OpCode::SLL: return OpCode::SLLI;
      case OpCode::SRL: return OpCode::SRLI;
      case OpCode::SRA: return OpCode::SRAI;
      case OpCode::MV: return OpCode::LI;
      default: assert(false); return OpCode::ADD;
    }
  }

  RISCV32InstGen &gen_;
};

}  // namespace mimic::back::asmgen::riscv32

#endif  // MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_IMMCONV_H_
