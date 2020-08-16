#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_SETCELIM_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_SETCELIM_H_

#include "back/asm/mir/pass.h"
#include "back/asm/arch/aarch32/instdef.h"
#include "back/asm/arch/aarch32/instgen.h"

namespace mimic::back::asmgen::aarch32 {

/*
  this pass will convert all SETCs to real instructions
*/
class SetCondEliminationPass : public PassInterface {
 public:
  SetCondEliminationPass(AArch32InstGen &gen) : gen_(gen) {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    // traverse all instructions
    for (auto it = insts.begin(); it != insts.end();) {
      auto inst = static_cast<AArch32Inst *>(it->get());
      switch (inst->opcode()) {
        case OpCode::SETEQ: case OpCode::SETNE: case OpCode::SETULT:
        case OpCode::SETSLT: case OpCode::SETULE: case OpCode::SETSLE:
        case OpCode::SETUGT: case OpCode::SETSGT: case OpCode::SETUGE:
        case OpCode::SETSGE: {
          it = HandleSetCond(insts, it, inst);
          continue;
        }
        default:;
      }
      ++it;
    }
  }

 private:
  using OpCode = AArch32Inst::OpCode;
  using InstIt = InstPtrList::iterator;

  template <typename... Args>
  InstIt InsertInst(InstPtrList &insts, InstIt pos, Args &&... args) {
    auto inst = std::make_shared<AArch32Inst>(std::forward<Args>(args)...);
    return ++insts.insert(pos, std::move(inst));
  }

  InstIt HandleSetCond(InstPtrList &insts, InstIt pos, AArch32Inst *setc) {
    if (setc->dest()->use_count()) {
      const auto &lhs = setc->oprs()[0].value();
      const auto &rhs = setc->oprs()[1].value();
      const auto &dest = setc->dest();
      auto temp = gen_.GetReg(AArch32Reg::RegName::R3);
      switch (setc->opcode()) {
        case OpCode::SETEQ: {
          pos = InsertInst(insts, pos, OpCode::SUB, temp, lhs, rhs);
          pos = InsertInst(insts, pos, OpCode::CLZ, temp, temp);
          pos = InsertInst(insts, pos, OpCode::LSR, dest, temp,
                           gen_.GetImm(5));
          break;
        }
        case OpCode::SETNE: {
          pos = InsertInst(insts, pos, OpCode::SUBS, temp, lhs, rhs);
          pos = InsertInst(insts, pos, OpCode::MOVWNE, temp,
                           gen_.GetImm(1));
          pos = InsertInst(insts, pos, OpCode::MOV, dest, temp);
          break;
        }
        default: {
          pos = InsertInst(insts, pos, OpCode::MOV, temp, gen_.GetImm(0));
          pos = InsertInst(insts, pos, OpCode::CMP, lhs, rhs);
          OpCode opcode;
          switch (setc->opcode()) {
            case OpCode::SETULT: opcode = OpCode::MOVWLO; break;
            case OpCode::SETSLT: opcode = OpCode::MOVWLT; break;
            case OpCode::SETULE: opcode = OpCode::MOVWLS; break;
            case OpCode::SETSLE: opcode = OpCode::MOVWLE; break;
            case OpCode::SETUGT: opcode = OpCode::MOVWHI; break;
            case OpCode::SETSGT: opcode = OpCode::MOVWGT; break;
            case OpCode::SETUGE: opcode = OpCode::MOVWHS; break;
            case OpCode::SETSGE: opcode = OpCode::MOVWGE; break;
            default: assert(false);
          }
          pos = InsertInst(insts, pos, opcode, temp, gen_.GetImm(1));
          pos = InsertInst(insts, pos, OpCode::MOV, dest, temp);
          break;
        }
      }
    }
    // remove current instruction
    return insts.erase(pos);
  }

  AArch32InstGen &gen_;
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_SETCELIM_H_
