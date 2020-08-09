#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_LEACOMB_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_LEACOMB_H_

#include <unordered_map>
#include <vector>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/aarch32/instdef.h"
#include "back/asm/arch/aarch32/instgen.h"

namespace mimic::back::asmgen::aarch32 {

/*
  this pass will combine LEA and LDR/STR
*/
class LeaCombiningPass : public PassInterface {
 public:
  LeaCombiningPass(AArch32InstGen &gen) : gen_(gen) {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    ResetSlots();
    // try to combine LEA and LDR/STR
    for (auto it = insts.begin(); it != insts.end();) {
      auto inst = static_cast<AArch32Inst *>(it->get());
      switch (inst->opcode()) {
        case OpCode::LEA: {
          // prevent current LEA from released by 'SimplifyLea'
          auto lea = *it;
          // try to simplify
          it = SimplifyLea(insts, it, inst);
          // add to slot definitions
          AddSlotDef(inst);
          continue;
        }
        case OpCode::LDR: case OpCode::LDRB: {
          if (auto slot = GetSlotDef(inst->oprs()[0].value())) {
            inst->oprs()[0].set_value(slot);
          }
          RemoveSlotDef(inst->dest());
          break;
        }
        case OpCode::STR: case OpCode::STRB: {
          if (auto slot = GetSlotDef(inst->oprs()[1].value())) {
            inst->oprs()[1].set_value(slot);
          }
          break;
        }
        default: {
          if (inst->IsLabel() || inst->IsCall()) {
            ResetSlots();
          }
          else if (inst->dest()) {
            RemoveSlotDef(inst->dest());
          }
          break;
        }
      }
      ++it;
    }
    // find dead LEAs
    leas_.clear();
    dead_leas_.clear();
    for (const auto &i : insts) {
      auto inst = static_cast<AArch32Inst *>(i.get());
      // used by other instructions, not dead
      for (const auto &opr : inst->oprs()) {
        if (opr.value()->IsReg()) leas_.erase(opr.value());
      }
      // overrided by other instructions, dead
      if (inst->dest()) {
        auto it = leas_.find(inst->dest());
        if (it != leas_.end()) {
          dead_leas_.push_back(it->second);
          leas_.erase(it);
        }
      }
      // new LEA instruction found
      if (inst->opcode() == OpCode::LEA) {
        // dest operand maybe function argument, not dead
        if (inst->dest()->IsReg() && !inst->dest()->IsVirtual()) {
          auto reg = static_cast<AArch32Reg *>(inst->dest().get())->name();
          switch (reg) {
            case RegName::R0: case RegName::R1: case RegName::R2:
            case RegName::R3: continue;
            default:;
          }
        }
        // add to map
        leas_.insert({inst->dest(), i});
      }
    }
    // remove dead LEAs
    for (const auto &[_, lea] : leas_) {
      insts.remove(lea);
    }
    for (const auto &lea : dead_leas_) {
      insts.remove(lea);
    }
  }

 private:
  using OpCode = AArch32Inst::OpCode;
  using RegName = AArch32Reg::RegName;
  using InstIt = InstPtrList::iterator;

  void ResetSlots() {
    slots_.clear();
  }

  InstIt SimplifyLea(InstPtrList &insts, InstIt pos, AArch32Inst *lea) {
    auto &ptr = lea->oprs()[0], &ofs = lea->oprs()[1];
    // get offset
    if (!ofs.value()->IsImm()) return ++pos;
    auto offset = static_cast<AArch32Imm *>(ofs.value().get())->val();
    // handle label first
    if (ptr.value()->IsLabel()) {
      auto ldr = std::make_shared<AArch32Inst>(OpCode::LDR, lea->dest(),
                                               ptr.value());
      if (!offset) {
        // replace with LDR
        *pos = std::move(ldr);
        return ++pos;
      }
      else {
        // insert LDR before current instruction
        pos = ++insts.insert(pos, std::move(ldr));
        ptr.set_value(lea->dest());
      }
    }
    else if (!offset) {
      return ++pos;
    }
    // handle by type
    if (ptr.value()->IsSlot()) {
      auto slot_ptr = static_cast<AArch32Slot *>(ptr.value().get());
      auto new_ofs = slot_ptr->offset() + offset;
      auto new_slot = gen_.GetSlot(slot_ptr->base(), new_ofs);
      ptr.set_value(new_slot);
      ofs.set_value(gen_.GetImm(0));
    }
    else if (ptr.value()->IsReg()) {
      auto base = ptr.value();
      if (base) {
        auto vreg_ptr = static_cast<VirtRegOperand *>(base.get());
        if (!vreg_ptr->alloc_to() || !vreg_ptr->alloc_to()->IsReg()) {
          return ++pos;
        }
        base = vreg_ptr->alloc_to();
      }
      auto slot = gen_.GetSlot(base, offset);
      ptr.set_value(slot);
      ofs.set_value(gen_.GetImm(0));
    }
    return ++pos;
  }

  void AddSlotDef(AArch32Inst *lea) {
    auto &ptr = lea->oprs()[0], &ofs = lea->oprs()[1];
    // check pointer
    if (!ptr.value()->IsSlot()) return;
    // check offset
    if (!ofs.value()->IsImm()) return;
    auto offset = static_cast<AArch32Imm *>(ofs.value().get())->val();
    if (offset > 4095 || offset < -4095) return;
    // add to definition
    slots_[lea->dest()] = ptr.value();
  }

  OprPtr GetSlotDef(const OprPtr &val) {
    auto it = slots_.find(val);
    return it != slots_.end() ? it->second : nullptr;
  }

  void RemoveSlotDef(const OprPtr &reg) {
    slots_.erase(reg);
  }

  AArch32InstGen &gen_;
  std::unordered_map<OprPtr, OprPtr> slots_;
  std::unordered_map<OprPtr, InstPtr> leas_;
  std::vector<InstPtr> dead_leas_;
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_LEACOMB_H_
