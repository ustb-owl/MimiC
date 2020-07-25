#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_BRELIM_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_BRELIM_H_

#include "back/asm/mir/pass.h"
#include "back/asm/arch/aarch32/instdef.h"

namespace mimic::back::asmgen::aarch32 {

class BranchEliminationPass : public PassInterface {
 public:
  BranchEliminationPass() {}

  void RunOn(InstPtrList &insts) override {
    using OpCode = AArch32Inst::OpCode;
    InstPtr last;
    for (auto it = insts.begin(); it != insts.end(); ++it) {
      if (last) {
        auto last_inst = static_cast<AArch32Inst *>(last.get());
        auto cur_inst = static_cast<AArch32Inst *>(it->get());
        // check if last instruction can be removed
        if (last_inst->opcode() == OpCode::B &&
            cur_inst->opcode() == OpCode::LABEL &&
            last_inst->oprs()[0] == cur_inst->oprs()[0]) {
          it = insts.erase(--it);
        }
      }
      // remember last instruction
      last = *it;
    }
  }
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_BRELIM_H_
