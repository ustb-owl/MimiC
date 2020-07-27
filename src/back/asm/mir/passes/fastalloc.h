#ifndef MIMIC_BACK_ASM_MIR_PASSES_FASTALLOC_H_
#define MIMIC_BACK_ASM_MIR_PASSES_FASTALLOC_H_

#include <queue>
#include <unordered_map>
#include <functional>
#include <cassert>

#include "back/asm/mir/passes/regalloc.h"

namespace mimic::back::asmgen {

/*
  fast register allocation

  TODO: this algorithm is wrong when processing loops
*/
class FastRegAllocPass : public RegAllocatorBase {
 public:
  FastRegAllocPass() {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    for (const auto &i : insts) {
      // retire operands
      for (auto &&use : i->oprs()) {
        // get virtual register info
        auto opr = use.value();
        if (!opr->IsVirtual()) continue;
        auto it = allocated_vregs_.find(opr);
        assert(it != allocated_vregs_.end());
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
        }
      }
      // check destination operand
      if (!i->dest() || !i->dest()->IsVirtual()) continue;
      OprPtr opr;
      auto it = allocated_vregs_.find(i->dest());
      if (it != allocated_vregs_.end()) {
        // use allocated register
        opr = it->second;
      }
      else {
        // allocate destination operand
        if (!unused_regs_.empty()) {
          opr = unused_regs_.front();
          unused_regs_.pop();
        }
        else if (!unused_slots_.empty()) {
          opr = unused_slots_.front();
          unused_slots_.pop();
        }
        else {
          opr = allocator().AllocateSlot(func_label);
        }
        // mark virtual register as allocated
        allocated_vregs_[i->dest()] = opr;
      }
      i->set_dest(opr);
    }
  }

  // add avaliable architecture
  void AddAvaliableReg(const OprPtr &reg) override {
    unused_slots_.push(reg);
  }

 private:
  // unused architecture regsters & retired stack slots
  std::queue<OprPtr> unused_regs_, unused_slots_;
  // allocated virtual registers
  std::unordered_map<OprPtr, OprPtr> allocated_vregs_;
};

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASSES_FASTALLOC_H_
