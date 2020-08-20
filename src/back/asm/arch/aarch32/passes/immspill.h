#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_IMMSPILL_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_IMMSPILL_H_

#include <cstdint>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/aarch32/instdef.h"
#include "back/asm/arch/aarch32/instgen.h"

namespace mimic::back::asmgen::aarch32 {

/*
  this pass will spill immediate value to virtual registers
*/
class ImmSpillPass : public PassInterface {
 public:
  ImmSpillPass(AArch32InstGen &gen) : gen_(gen) {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    for (auto it = insts.begin(); it != insts.end(); ++it) {
      auto inst = static_cast<AArch32Inst *>(it->get());
      switch (inst->opcode()) {
        // instructions that allow register operands only
        case OpCode::STR: case OpCode::STRB: case OpCode::MUL:
        case OpCode::MLS: case OpCode::SMMUL: case OpCode::UMULL:
        case OpCode::SDIV: case OpCode::UDIV: case OpCode::CLZ:
        case OpCode::SXTB: case OpCode::UXTB: {
          for (auto &&i : inst->oprs()) {
            if (i.value()->IsImm()) {
              auto temp = gen_.GetVReg();
              InsertMove(insts, it, i.value(), temp);
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
  using OpCode = AArch32Inst::OpCode;
  using RegName = AArch32Reg::RegName;

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

  InstPtr MakeMove(const OprPtr &dest, const OprPtr &src) {
    return std::make_shared<AArch32Inst>(OpCode::MOV, dest, src);
  }

  InstPtr MakeMoveHi(const OprPtr &dest, const OprPtr &src) {
    return std::make_shared<AArch32Inst>(OpCode::MOVT, dest, src);
  }

  AArch32InstGen &gen_;
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_IMMSPILL_H_
