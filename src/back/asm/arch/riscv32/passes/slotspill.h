#ifndef MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_SLOTSPILL_H_
#define MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_SLOTSPILL_H_

#include <cstdint>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/riscv32/instdef.h"
#include "back/asm/arch/riscv32/instgen.h"

namespace mimic::back::asmgen::riscv32 {

/*
  this pass will:
  1.  apply the allocation results of virtual registers
  2.  add loads/stores for spilled virtual registers
*/
class SlotSpillingPass : public PassInterface {
 public:
  SlotSpillingPass(RISCV32InstGen &gen) : gen_(gen) {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    for (auto it = insts.begin(); it != insts.end(); ++it) {
      auto inst = *it;
      // handle with source operands
      if (inst->IsMove() && inst->oprs()[0].value()->IsVirtual()) {
        auto &opr = inst->oprs()[0];
        const auto &alloc_to = GetAllocTo(opr.value());
        if (alloc_to->IsSlot()) {
          // insert load to dest and remove current move
          InsertLoad(insts, it, alloc_to, inst->dest());
          it = --insts.erase(it);
          inst = *it;
        }
        else {
          // just replace
          opr.set_value(alloc_to);
        }
      }
      else if (!inst->IsMove()) {
        auto reg_mask = GetRegMask(inst);
        for (auto &&i : inst->oprs()) {
          if (!i.value()->IsVirtual()) continue;
          const auto &alloc_to = GetAllocTo(i.value());
          if (alloc_to->IsReg()) {
            i.set_value(alloc_to);
          }
          else {
            auto dest = SelectTempReg(reg_mask);
            InsertLoad(insts, it, alloc_to, dest);
            i.set_value(dest);
          }
        }
      }
      // handle with destination operands
      if (inst->dest() && inst->dest()->IsVirtual()) {
        const auto &alloc_to = GetAllocTo(inst->dest());
        if (alloc_to->IsReg()) {
          inst->set_dest(alloc_to);
        }
        else {
          auto temp = gen_.GetReg(RegName::T0);
          inst->set_dest(temp);
          InsertStore(insts, it, alloc_to, temp);
        }
      }
    }
  }

 private:
  using OpCode = RISCV32Inst::OpCode;
  using RegName = RISCV32Reg::RegName;

  std::uint32_t GetRegMask(const InstPtr &inst) {
    std::uint32_t mask = 0;
    for (const auto &i : inst->oprs()) {
      const auto &opr = i.value();
      if (opr->IsReg() && !opr->IsVirtual()) {
        auto name = static_cast<RISCV32Reg *>(opr.get())->name();
        mask |= 1 << static_cast<int>(name);
      }
    }
    return mask;
  }

  OprPtr SelectTempReg(std::uint32_t &reg_mask) {
    OprPtr temp;
    for (int i = static_cast<int>(RegName::T0);
         i <= static_cast<int>(RegName::T2); ++i) {
      if (!(reg_mask & (1 << i))) {
        reg_mask |= 1 << i;
        temp = gen_.GetReg(static_cast<RegName>(i));
        break;
      }
    }
    assert(temp);
    return temp;
  }

  // insert a load instruction before the specific position
  void InsertLoad(InstPtrList &insts, InstPtrList::iterator &pos,
                  const OprPtr &slot, const OprPtr &dest) {
    // get slot info
    assert(slot->IsSlot() && dest->IsReg());
    auto sl = static_cast<RISCV32Slot *>(slot.get());
    assert(sl->base() == gen_.GetReg(RegName::FP) && sl->offset() < 0);
    // generate load
    InstPtr inst;
    if (-sl->offset() >= 2048) {
      // calculate address of slot first
      auto fp = gen_.GetReg(RegName::FP);
      auto ofs = gen_.GetImm(-sl->offset());
      auto temp = dest->IsVirtual() ? gen_.GetReg(RegName::T1) : dest;
      inst = std::make_shared<RISCV32Inst>(OpCode::SUB, temp, fp, ofs);
      pos = ++insts.insert(pos, inst);
      inst = std::make_shared<RISCV32Inst>(OpCode::LW, dest, temp);
    }
    else {
      inst = std::make_shared<RISCV32Inst>(OpCode::LW, dest, slot);
    }
    pos = ++insts.insert(pos, inst);
  }

  // insert a store instruction after the specific position
  void InsertStore(InstPtrList &insts, InstPtrList::iterator &pos,
                   const OprPtr &slot, const OprPtr &dest) {
    // get slot info
    // TODO: do not hard code the argument 'dest'
    assert(slot->IsSlot() && dest->IsReg() &&
           static_cast<RISCV32Reg *>(dest.get())->name() == RegName::T0);
    auto sl = static_cast<RISCV32Slot *>(slot.get());
    assert(sl->base() == gen_.GetReg(RegName::FP) && sl->offset() < 0);
    // generate store
    InstPtr inst;
    if (-sl->offset() >= 2048) {
      // calculate address of slot first
      auto temp = gen_.GetReg(RegName::T1);
      auto fp = gen_.GetReg(RegName::FP);
      auto ofs = gen_.GetImm(-sl->offset());
      inst = std::make_shared<RISCV32Inst>(OpCode::SUB, temp, fp, ofs);
      pos = insts.insert(++pos, inst);
      inst = std::make_shared<RISCV32Inst>(OpCode::SW, dest, temp);
    }
    else {
      inst = std::make_shared<RISCV32Inst>(OpCode::SW, dest, slot);
    }
    pos = insts.insert(++pos, inst);
  }

  const OprPtr &GetAllocTo(const OprPtr &vreg) {
    assert(vreg->IsVirtual());
    return static_cast<VirtRegOperand *>(vreg.get())->alloc_to();
  }

  RISCV32InstGen &gen_;
};

}  // namespace mimic::back::asmgen::riscv32

#endif  // MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_SLOTSPILL_H_
