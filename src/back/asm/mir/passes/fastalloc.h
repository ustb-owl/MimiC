#ifndef MIMIC_BACK_ASM_MIR_PASSES_FASTALLOC_H_
#define MIMIC_BACK_ASM_MIR_PASSES_FASTALLOC_H_

#include <queue>
#include <unordered_map>
#include <functional>
#include <cassert>

#include "back/asm/mir/pass.h"

namespace mimic::back::asmgen {

/*
  fast register allocation
*/
class FastRegAllocPass : public PassInterface {
 public:
  FastRegAllocPass() {}

  void RunOn(InstPtrList &insts) override {
    for (const auto &i : insts) {
      // retire operands
      for (auto &&use : i->oprs()) {
        // get virtual register info
        auto opr = use.value();
        if (!opr->IsVirtual()) continue;
        auto it = active_vregs_.find(opr);
        assert(it != active_vregs_.end());
        // replace with allocated operand
        use.set_value(it->second);
        if (!opr->use_count()) {
          // current virtual register is inactive
          if (it->second->IsReg()) {
            unused_regs_.push(it->second);
          }
          else {
            assert(it->second->IsSlot());
            unused_slots_.push(it->second);
          }
          active_vregs_.erase(it);
        }
      }
      // check destination operand
      if (!i->dest() || !i->dest()->IsVirtual()) continue;
      // allocate destination operand
      OprPtr opr;
      if (!unused_regs_.empty()) {
        opr = unused_regs_.front();
        unused_regs_.pop();
      }
      else if (!unused_slots_.empty()) {
        opr = unused_slots_.front();
        unused_slots_.pop();
      }
      else {
        opr = alloc_slot_();
      }
      // mark virtual register as active
      active_vregs_[i->dest()] = opr;
      i->set_dest(opr);
    }
  }

  // add avaliable architecture
  void AddAvaliableReg(const OprPtr &reg) { unused_slots_.push(reg); }

  // setters
  void set_alloc_slot(std::function<OprPtr()> alloc_slot) {
    alloc_slot_ = alloc_slot;
  }

 private:
  // unused architecture regsters & retired stack slots
  std::queue<OprPtr> unused_regs_, unused_slots_;
  // active virtual registers
  std::unordered_map<OprPtr, OprPtr> active_vregs_;
  // stack slot allocator
  std::function<OprPtr()> alloc_slot_;
};

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASSES_FASTALLOC_H_
