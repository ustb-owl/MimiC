#ifndef MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_LEAELIM_H_
#define MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_LEAELIM_H_

#include <utility>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/riscv32/instdef.h"
#include "back/asm/arch/riscv32/instgen.h"

namespace mimic::back::asmgen::riscv32 {

/*
  this pass will convert all LEAs to real instructions
*/
class LeaEliminationPass : public PassInterface {
 public:
  LeaEliminationPass(RISCV32InstGen &gen) : gen_(gen) {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) {
    for (auto it = insts.begin(); it != insts.end();) {
      auto inst = static_cast<RISCV32Inst *>(it->get());
      if (inst->opcode() == OpCode::LEA) {
        it = HandleLea(insts, it, inst);
      }
      else {
        ++it;
      }
    }
  }

 private:
  using OpCode = RISCV32Inst::OpCode;
  using RegName = RISCV32Reg::RegName;
  using InstIt = InstPtrList::iterator;

  template <typename... Args>
  InstIt InsertBefore(InstPtrList &insts, InstIt pos, Args &&... args) {
    auto inst = std::make_shared<RISCV32Inst>(std::forward<Args>(args)...);
    return ++insts.insert(pos, std::move(inst));
  }

  InstIt HandleLea(InstPtrList &insts, InstIt pos, RISCV32Inst *lea) {
    auto &ptr = lea->oprs()[0].value(), &offset = lea->oprs()[1].value();
    const auto &dest = lea->dest();
    // check if 'offset' is immediate zero
    bool ofs_zero = offset->IsImm() &&
                    !static_cast<RISCV32Imm *>(offset.get())->val();
    const auto &temp = ofs_zero ? dest : gen_.GetReg(RegName::A7);
    // handle by pointer type
    if (ptr->IsSlot()) {
      // get base register
      auto p = static_cast<RISCV32Slot *>(ptr.get());
      pos = InsertBefore(insts, pos, OpCode::MV, temp, p->base());
      // calculate stack offset
      auto ofs = p->offset();
      if (ofs > 0) {
        pos = InsertBefore(insts, pos, OpCode::ADD, temp, temp,
                           gen_.GetImm(ofs));
      }
      else if (ofs < 0) {
        pos = InsertBefore(insts, pos, OpCode::SUB, temp, temp,
                           gen_.GetImm(-ofs));
      }
    }
    else if (ptr->IsLabel()) {
      // load label address
      pos = InsertBefore(insts, pos, OpCode::LA, temp, ptr);
    }
    else {
      assert(ptr->IsReg());
      // just move address to destination
      pos = InsertBefore(insts, pos, OpCode::MV, temp, ptr);
    }
    // add offset to result if offset is not zero
    if (!ofs_zero) {
      pos = InsertBefore(insts, pos, OpCode::ADD, dest, temp, offset);
    }
    // erase the original LEA
    return insts.erase(pos);
  }

  RISCV32InstGen &gen_;
};

}  // namespace mimic::back::asmgen::riscv32

#endif  // MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_LEAELIM_H_
