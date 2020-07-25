#ifndef MIMIC_BACK_ASM_MIR_PASSES_MOVELIM_H_
#define MIMIC_BACK_ASM_MIR_PASSES_MOVELIM_H_

#include "back/asm/mir/pass.h"

namespace mimic::back::asmgen {

class MoveEliminatePass : public PassInterface {
 public:
  MoveEliminatePass() {}

  void RunOn(InstPtrList &insts) override {
    InstPtr last;
    for (auto it = insts.begin(); it != insts.end();) {
      if (last && (*it)->IsMove()) {
        // check if current instruction needs to be removed
        if ((*it)->oprs()[0].value() == last->dest()) {
          last->set_dest((*it)->dest());
          it = insts.erase(it);
          continue;
        }
      }
      // remember last instruction
      last = *it;
      ++it;
    }
  }
};

}  // namespace mimic::back::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASSES_MOVELIM_H_
