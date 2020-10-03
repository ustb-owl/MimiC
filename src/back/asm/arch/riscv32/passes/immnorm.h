#ifndef MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_IMMNORM_H_
#define MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_IMMNORM_H_

#include <cstdint>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/riscv32/instdef.h"
#include "back/asm/arch/riscv32/instgen.h"

namespace mimic::back::asmgen::riscv32 {

/*
  this pass will:
  1.  insert move instructions to load immediate that out of range
  2.  convert instructions like 'add r0, #3, r0' to legal form
*/
class ImmNormalizePass : public PassInterface {
 public:
  ImmNormalizePass(RISCV32InstGen &gen) : gen_(gen) {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    for (auto it = insts.begin(); it != insts.end(); ++it) {
      auto inst = static_cast<RISCV32Inst *>(it->get());
      switch (inst->opcode()) {
        // instructions with imm[11:0] field
        case OpCode::ADDI: case OpCode::SLTI: case OpCode::SLTIU:
        case OpCode::XORI: case OpCode::ORI: case OpCode::ANDI: {
          auto mask = GetRegMask(inst);
          for (std::size_t i = 0; i < inst->oprs().size(); ++i) {
            auto &cur = inst->oprs()[i];
            if ((i < inst->oprs().size() - 1 && cur.value()->IsImm()) ||
                (i == inst->oprs().size() - 1 &&
                 !IsValidOpr12(cur.value()))) {
              auto temp = SelectTempReg(mask);
              InsertLI(insts, it, cur.value(), temp);
              cur.set_value(temp);
            }
          }
          break;
        }
        // instructions with only imm[4:0] field
        case OpCode::SLLI: case OpCode::SRLI: case OpCode::SRAI: {
          auto mask = GetRegMask(inst);
          for (std::size_t i = 0; i < inst->oprs().size(); ++i) {
            auto &cur = inst->oprs()[i];
            if ((i < inst->oprs().size() - 1 && cur.value()->IsImm()) ||
                (i == inst->oprs().size() - 1 &&
                 !IsValidOpr5(cur.value()))) {
              auto temp = SelectTempReg(mask);
              InsertLI(insts, it, cur.value(), temp);
              cur.set_value(temp);
            }
          }
          break;
        }
        // instructions that allow register operands only
        case OpCode::ADD: case OpCode::SUB: case OpCode::SLT:
        case OpCode::SLTU: case OpCode::MUL: case OpCode::DIV:
        case OpCode::DIVU: case OpCode::REM: case OpCode::REMU:
        case OpCode::BEQ: case OpCode::BNE: case OpCode::BLT:
        case OpCode::BLE: case OpCode::BGT: case OpCode::BGE:
        case OpCode::BLTU: case OpCode::BLEU: case OpCode::BGTU:
        case OpCode::BGEU: case OpCode::XOR: case OpCode::OR:
        case OpCode::AND: case OpCode::SLL: case OpCode::SRL:
        case OpCode::SRA: {
          auto mask = GetRegMask(inst);
          for (auto &&i : inst->oprs()) {
            if (i.value()->IsImm()) {
              auto temp = SelectTempReg(mask);
              InsertLI(insts, it, i.value(), temp);
              i.set_value(temp);
            }
          }
          break;
        }
        // other instructions, don't care
        default:;
      }
    }
  }

 private:
  using OpCode = RISCV32Inst::OpCode;
  using RegName = RISCV32Reg::RegName;

  std::uint32_t GetRegMask(InstBase *inst) {
    std::uint32_t mask = 0;
    for (const auto &i : inst->oprs()) {
      const auto &opr = i.value();
      if (opr->IsReg()) {
        assert(!opr->IsVirtual());
        auto name = static_cast<RISCV32Reg *>(opr.get())->name();
        mask |= 1 << static_cast<int>(name);
      }
    }
    return mask;
  }

  OprPtr SelectTempReg(std::uint32_t &reg_mask) {
    OprPtr temp;
    for (int i = static_cast<int>(RegName::A6);
         i <= static_cast<int>(RegName::A7); ++i) {
      if (!(reg_mask & (1 << i))) {
        reg_mask |= 1 << i;
        temp = gen_.GetReg(static_cast<RegName>(i));
      }
    }
    assert(temp);
    return temp;
  }

  void InsertLI(InstPtrList &insts, InstPtrList::iterator &pos,
                  const OprPtr &opr, const OprPtr &dest) {
    // get value of immediate
    assert(opr->IsImm());
    std::uint32_t imm = static_cast<RISCV32Imm *>(opr.get())->val();
    auto imm_opr = gen_.GetImm(imm);
    // insert load immediate
    auto li = std::make_shared<RISCV32Inst>(OpCode::LI, dest, imm_opr);
    pos = ++insts.insert(pos, li);
  }

  bool IsValidOpr12(const OprPtr &opr) {
    if (!opr->IsImm()) return true;
    std::int32_t imm = static_cast<RISCV32Imm *>(opr.get())->val();
    return imm >= -2048 && imm <= 2047;
  }

  bool IsValidOpr5(const OprPtr &opr) {
    if (!opr->IsImm()) return true;
    std::uint32_t imm = static_cast<RISCV32Imm *>(opr.get())->val();
    return imm <= 0b11111;
  }

  RISCV32InstGen &gen_;
};

}  // namespace mimic::back::asmgen::riscv32

#endif  // MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_IMMNORM_H_
