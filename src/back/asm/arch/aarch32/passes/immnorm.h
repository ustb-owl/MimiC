#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_IMMNORM_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_IMMNORM_H_

#include <cstdint>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/aarch32/instdef.h"
#include "back/asm/arch/aarch32/instgen.h"

namespace mimic::back::asmgen::aarch32 {

/*
  this pass will:
  1.  insert move instructions to load immediate that out of range
  2.  convert instructions like 'add r0, #3, r0' to legal form
  3.  convert 'ldr r0, =label' to move instructions
*/
class ImmNormalizePass : public PassInterface {
 public:
  ImmNormalizePass(AArch32InstGen &gen) : gen_(gen) {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    for (auto it = insts.begin(); it != insts.end(); ++it) {
      auto inst = static_cast<AArch32Inst *>(it->get());
      switch (inst->opcode()) {
        // unconditional move instruction
        case OpCode::MOV: {
          if (!IsValidOpr16(inst->oprs()[0].value())) {
            InsertMove(insts, it, inst->oprs()[0].value(), inst->dest());
            it = --insts.erase(it);
          }
          break;
        }
        // instructions that allow register operands only
        case OpCode::STR: case OpCode::STRB: case OpCode::MUL:
        case OpCode::MLS: case OpCode::SDIV: case OpCode::UDIV:
        case OpCode::CLZ: case OpCode::SXTB: case OpCode::UXTB: {
          auto mask = GetRegMask(inst);
          for (auto &&i : inst->oprs()) {
            if (i.value()->IsImm()) {
              auto temp = SelectTempReg(mask);
              InsertMove(insts, it, i.value(), temp);
              i.set_value(temp);
            }
          }
          break;
        }
        // instructions with <imm8m> & <imm12> field
        case OpCode::ADD: case OpCode::SUB: {
          auto mask = GetRegMask(inst);
          for (std::size_t i = 0; i < inst->oprs().size(); ++i) {
            auto &cur = inst->oprs()[i];
            if ((i < inst->oprs().size() - 1 && cur.value()->IsImm()) ||
                (i == inst->oprs().size() - 1 &&
                 !IsValidOpr12(cur.value()))) {
              auto temp = SelectTempReg(mask);
              InsertMove(insts, it, cur.value(), temp);
              cur.set_value(temp);
            }
          }
          break;
        }
        // instructions with only <imm8m> field
        case OpCode::SUBS: case OpCode::RSB: case OpCode::CMP:
        case OpCode::AND: case OpCode::ORR: case OpCode::EOR: {
          auto mask = GetRegMask(inst);
          for (std::size_t i = 0; i < inst->oprs().size(); ++i) {
            auto &cur = inst->oprs()[i];
            if ((i < inst->oprs().size() - 1 && cur.value()->IsImm()) ||
                (i == inst->oprs().size() - 1 &&
                 !IsValidOpr8m(cur.value()))) {
              auto temp = SelectTempReg(mask);
              InsertMove(insts, it, cur.value(), temp);
              cur.set_value(temp);
            }
          }
          break;
        }
        // instructions with only <Rs|sh> field
        case OpCode::LSL: case OpCode::LSR: case OpCode::ASR: {
          auto mask = GetRegMask(inst);
          for (std::size_t i = 0; i < inst->oprs().size(); ++i) {
            auto &cur = inst->oprs()[i];
            if ((i < inst->oprs().size() - 1 && cur.value()->IsImm()) ||
                (i == inst->oprs().size() - 1 &&
                 !IsValidOprSh(cur.value()))) {
              auto temp = SelectTempReg(mask);
              InsertMove(insts, it, cur.value(), temp);
              cur.set_value(temp);
            }
          }
          break;
        }
        // load instruction with label operand
        case OpCode::LDR: {
          const auto &opr = inst->oprs()[0].value();
          if (!opr->IsLabel()) break;
          // insert move instructions
          it = ++insts.insert(it, MakeMoveW(inst->dest(), opr));
          it = ++insts.insert(it, MakeMoveHi(inst->dest(), opr));
          // remove current instruction
          it = --insts.erase(it);
          break;
        }
        // other instructions, don't care
        default:;
      }
    }
  }

 private:
  using OpCode = AArch32Inst::OpCode;
  using RegName = AArch32Reg::RegName;

  std::uint32_t GetRegMask(InstBase *inst) {
    std::uint32_t mask = 0;
    for (const auto &i : inst->oprs()) {
      const auto &opr = i.value();
      if (opr->IsReg()) {
        assert(!opr->IsVirtual());
        auto name = static_cast<AArch32Reg *>(opr.get())->name();
        mask |= 1 << static_cast<int>(name);
      }
    }
    return mask;
  }

  OprPtr SelectTempReg(std::uint32_t &reg_mask) {
    OprPtr temp;
    for (int i = static_cast<int>(RegName::R0);
         i <= static_cast<int>(RegName::R3); ++i) {
      if (!(reg_mask & (1 << i))) {
        reg_mask |= 1 << i;
        temp = gen_.GetReg(static_cast<RegName>(i));
        break;
      }
    }
    assert(temp);
    return temp;
  }

  void InsertMove(InstPtrList &insts, InstPtrList::iterator &pos,
                  const OprPtr &opr, const OprPtr &dest) {
    // get value of immediate
    assert(opr->IsImm());
    std::uint32_t imm = static_cast<AArch32Imm *>(opr.get())->val();
    // insert move
    if (IsValidImm8m(imm)) {
      pos = ++insts.insert(pos, MakeMove(dest, gen_.GetImm(imm)));
    }
    else {
      auto lo = imm & 0xffff, hi = imm >> 16;
      pos = ++insts.insert(pos, MakeMove(dest, gen_.GetImm(lo)));
      if (hi) pos = ++insts.insert(pos, MakeMoveHi(dest, gen_.GetImm(hi)));
    }
  }

  // check if immediate is a valid <imm8m> literal
  // mother fxcking ARM
  bool IsValidImm8m(std::uint32_t imm) {
    for (int i = 0; i < 16; ++i) {
      // perform circular shift 2i bits
      std::uint32_t cur = (i << 1) & 0b11111;
      cur = (imm << cur) | (imm >> ((-cur) & 0b11111));
      // check if is valid
      if (!(cur & ~0xff)) return true;
    }
    return false;
  }

  bool IsValidOpr8m(const OprPtr &opr) {
    if (!opr->IsImm()) return true;
    std::uint32_t imm = static_cast<AArch32Imm *>(opr.get())->val();
    return IsValidImm8m(imm);
  }

  bool IsValidOpr16(const OprPtr &opr) {
    if (!opr->IsImm()) return true;
    std::uint32_t imm = static_cast<AArch32Imm *>(opr.get())->val();
    return imm <= 0xffff || IsValidImm8m(imm);
  }

  bool IsValidOpr12(const OprPtr &opr) {
    if (!opr->IsImm()) return true;
    std::uint32_t imm = static_cast<AArch32Imm *>(opr.get())->val();
    return imm <= 0xfff || IsValidImm8m(imm);
  }

  bool IsValidOprSh(const OprPtr &opr) {
    if (!opr->IsImm()) return true;
    std::uint32_t imm = static_cast<AArch32Imm *>(opr.get())->val();
    return imm <= 0b11111;
  }

  InstPtr MakeMove(const OprPtr &dest, const OprPtr &src) {
    return std::make_shared<AArch32Inst>(OpCode::MOV, dest, src);
  }

  InstPtr MakeMoveW(const OprPtr &dest, const OprPtr &src) {
    return std::make_shared<AArch32Inst>(OpCode::MOVW, dest, src);
  }

  InstPtr MakeMoveHi(const OprPtr &dest, const OprPtr &src) {
    return std::make_shared<AArch32Inst>(OpCode::MOVT, dest, src);
  }

  AArch32InstGen &gen_;
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_IMMNORM_H_
