#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_SLOTSPILL_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_SLOTSPILL_H_

#include <cstdint>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/aarch32/instdef.h"
#include "back/asm/arch/aarch32/instgen.h"

namespace mimic::back::asmgen::aarch32 {

/*
  this pass will add loads/stores for spilled virtual registers
*/
class SlotSpillingPass : public PassInterface {
 public:
  SlotSpillingPass(const AArch32InstGen &gen) : gen_(gen) {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    for (auto it = insts.begin(); it != insts.end(); ++it) {
      auto inst = static_cast<AArch32Inst *>(it->get());
      // handle with source operands
      auto reg_mask = GetRegMask(inst);
      if (inst->opcode() != OpCode::LDR && inst->opcode() != OpCode::LDRB) {
        if (inst->IsMove()) {
          auto &opr = inst->oprs()[0];
          auto reg = LoadOperand(insts, it, opr.value(), reg_mask);
          if (reg) {
            // remove current move, just use load
            (*--it)->set_dest(inst->dest());
            it = --insts.erase(++it);
            inst = static_cast<AArch32Inst *>(it->get());
          }
        }
        else {
          for (auto &&i : inst->oprs()) {
            auto reg = LoadOperand(insts, it, i.value(), reg_mask);
            if (reg) i.set_value(reg);
          }
        }
      }
      // handle with destination operands
      if (inst->dest()) {
        auto reg = StoreOperand(insts, it, inst->dest());
        if (reg) inst->set_dest(reg);
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

  OprPtr LoadOperand(InstPtrList &insts, InstPtrList::iterator &pos,
                     const OprPtr &opr, std::uint32_t &reg_mask) {
    if (!opr->IsSlot()) return nullptr;
    // select temporary register
    OprPtr temp;
    for (int i = static_cast<int>(RegName::R1);
         i <= static_cast<int>(RegName::R3); ++i) {
      if (!(reg_mask & (1 << i))) {
        reg_mask |= 1 << i;
        temp = gen_.GetReg(static_cast<RegName>(i));
        break;
      }
    }
    assert(temp);
    // insert load
    auto inst = std::make_shared<AArch32Inst>(OpCode::LDR, temp, opr);
    pos = ++insts.insert(pos, inst);
    return temp;
  }

  OprPtr StoreOperand(InstPtrList &insts, InstPtrList::iterator &pos,
                      const OprPtr &dest) {
    if (!dest->IsSlot()) return nullptr;
    auto temp = gen_.GetReg(RegName::R1);
    // insert store
    auto inst = std::make_shared<AArch32Inst>(OpCode::STR, temp, dest);
    pos = insts.insert(++pos, inst);
    return temp;
  }

  const AArch32InstGen &gen_;
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_SLOTSPILL_H_
