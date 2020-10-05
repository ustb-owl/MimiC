#ifndef MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_LEACOMB_H_
#define MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_LEACOMB_H_

#include <unordered_map>
#include <vector>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/riscv32/instdef.h"
#include "back/asm/arch/riscv32/instgen.h"

namespace mimic::back::asmgen::riscv32 {

/*
  this pass will combine LEA and LW/SW
*/
class LeaCombiningPass : public PassInterface {
 public:
  LeaCombiningPass(RISCV32InstGen &gen) : gen_(gen) {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    ResetSlots();
    // try to combine LEA and LW/SW
    for (auto it = insts.begin(); it != insts.end();) {
      auto inst = static_cast<RISCV32Inst *>(it->get());
      switch (inst->opcode()) {
        case OpCode::LEA: {
          // prevent current LEA from released by 'SimplifyLea'
          auto lea = *it;
          // try to simplify
          it = SimplifyLea(insts, it, inst);
          // add to slot definitions
          RemoveSlotDef(inst->dest());
          RemoveUsedByDef(inst->dest());
          AddSlotDef(inst);
          continue;
        }
        case OpCode::LW: case OpCode::LB: case OpCode::LBU: {
          if (auto slot = GetSlotDef(inst->oprs()[0].value())) {
            inst->oprs()[0].set_value(slot);
          }
          RemoveSlotDef(inst->dest());
          RemoveUsedByDef(inst->dest());
          break;
        }
        case OpCode::SW: case OpCode::SB: {
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
          RemoveUsedByDef(inst->dest());
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
      auto inst = static_cast<RISCV32Inst *>(i.get());
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
          auto reg = static_cast<RISCV32Reg *>(inst->dest().get())->name();
          if (static_cast<int>(reg) >= static_cast<int>(RegName::A0) &&
              static_cast<int>(reg) <= static_cast<int>(RegName::A7)) {
            continue;
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
  using OpCode = RISCV32Inst::OpCode;
  using RegName = RISCV32Reg::RegName;
  using InstIt = InstPtrList::iterator;

  OprPtr GetRealReg(const OprPtr &reg) {
    assert(reg->IsReg());
    if (reg->IsVirtual()) {
      auto vreg_ptr = static_cast<VirtRegOperand *>(reg.get());
      if (vreg_ptr->alloc_to() && vreg_ptr->alloc_to()->IsReg()) {
        return vreg_ptr->alloc_to();
      }
    }
    return reg;
  }

  void ResetSlots() {
    slots_.clear();
    uses_.clear();
  }

  InstIt SimplifyLea(InstPtrList &insts, InstIt pos, RISCV32Inst *lea) {
    auto &ptr = lea->oprs()[0], &ofs = lea->oprs()[1];
    // handle label first
    bool ofs_zero = ofs.value()->IsImm() &&
                    !static_cast<RISCV32Imm *>(ofs.value().get())->val();
    if (ptr.value()->IsLabel()) {
      auto la = std::make_shared<RISCV32Inst>(OpCode::LA, lea->dest(),
                                              ptr.value());
      if (ofs_zero) {
        // replace with LA
        *pos = std::move(la);
        return ++pos;
      }
      else {
        // insert LA before current instruction
        pos = ++insts.insert(pos, std::move(la));
        ptr.set_value(lea->dest());
      }
    }
    // get offset
    if (!ofs.value()->IsImm()) return ++pos;
    auto offset = static_cast<RISCV32Imm *>(ofs.value().get())->val();
    if (!offset) return ++pos;
    // handle by type
    if (ptr.value()->IsSlot()) {
      auto slot_ptr = static_cast<RISCV32Slot *>(ptr.value().get());
      auto new_ofs = slot_ptr->offset() + offset;
      auto new_slot = gen_.GetSlot(slot_ptr->base(), new_ofs);
      ptr.set_value(new_slot);
      ofs.set_value(gen_.GetImm(0));
    }
    else if (ptr.value()->IsReg()) {
      auto base = ptr.value();
      if (base->IsVirtual()) {
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

  void AddSlotDef(RISCV32Inst *lea) {
    auto &ptr = lea->oprs()[0].value(), &ofs = lea->oprs()[1].value();
    // check pointer
    if (!ptr->IsSlot()) return;
    auto slot_ptr = static_cast<RISCV32Slot *>(ptr.get());
    // check offset
    if (!ofs->IsImm()) return;
    auto offset = static_cast<RISCV32Imm *>(ofs.get())->val();
    if (offset > 2047 || offset < -2048) return;
    // add to definition
    auto dest = GetRealReg(lea->dest());
    slots_.insert({dest, ptr});
    uses_.insert({slot_ptr->base(), dest});
  }

  OprPtr GetSlotDef(const OprPtr &val) {
    if (!val->IsReg()) return nullptr;
    auto it = slots_.find(GetRealReg(val));
    return it != slots_.end() ? it->second : nullptr;
  }

  void RemoveSlotDef(const OprPtr &reg) {
    auto real = GetRealReg(reg);
    auto it = slots_.find(real);
    if (it != slots_.end()) {
      // remove from uses map
      auto slot_ptr = static_cast<RISCV32Slot *>(it->second.get());
      auto [cur, end] = uses_.equal_range(slot_ptr->base());
      for (; cur != end; ++cur) {
        if (cur->second == real) {
          uses_.erase(cur);
          break;
        }
      }
      // remove from definitions
      slots_.erase(it);
    }
  }

  void RemoveUsedByDef(const OprPtr &base) {
    auto [begin, end] = uses_.equal_range(GetRealReg(base));
    // remove all definitions with specific value
    for (auto it = begin; it != end; ++it) {
      slots_.erase(it->second);
    }
    // remove expired uses
    uses_.erase(begin, end);
  }

  RISCV32InstGen &gen_;
  std::unordered_map<OprPtr, OprPtr> slots_;
  std::unordered_multimap<OprPtr, OprPtr> uses_;
  std::unordered_map<OprPtr, InstPtr> leas_;
  std::vector<InstPtr> dead_leas_;
};

}  // namespace mimic::back::asmgen::riscv32

#endif  // MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_LEACOMB_H_
