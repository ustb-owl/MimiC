#ifndef MIMIC_BACK_ASM_MIR_PASSES_MOVOVERRIDE_H_
#define MIMIC_BACK_ASM_MIR_PASSES_MOVOVERRIDE_H_

#include "back/asm/mir/pass.h"

namespace mimic::back::asmgen {

/*
  this pass will remove move instructions overridden by other instructions
*/
class MoveOverridingPass : public PassInterface {
 public:
  MoveOverridingPass() {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    InstPtr last;
    for (auto it = insts.begin(); it != insts.end();) {
      if ((*it)->IsMove() && ((*it)->dest() == (*it)->oprs()[0].value())) {
        // redundant move, just remove
        it = insts.erase(it);
      }
      else if (last && last->IsMove()) {
        auto cur = *it;
        it = CheckAndRemove(insts, it, last);
        last = cur;
      }
      else {
        last = *it;
        ++it;
      }
    }
  }

 private:
  using InstIt = InstPtrList::iterator;

  InstIt CheckAndRemove(InstPtrList &insts, InstIt pos,
                        const InstPtr &last) {
    // must be a register-to-register move
    if (!last->dest()->IsReg() || !last->oprs()[0].value()->IsReg()) {
      return ++pos;
    }
    // current instruction must override last move instruction
    if ((*pos)->dest() != last->dest()) return ++pos;
    // current instruction must not use 'dest' of last instruction
    for (const auto &opr : (*pos)->oprs()) {
      if (opr.value() == last->dest()) return ++pos;
    }
    // remove last instruction
    return ++insts.erase(--pos);
  }
};

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASSES_MOVOVERRIDE_H_
