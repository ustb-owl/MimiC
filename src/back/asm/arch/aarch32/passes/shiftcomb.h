#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_SHIFTCOMB_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_SHIFTCOMB_H_

#include <unordered_map>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/aarch32/instdef.h"

namespace mimic::back::asmgen::aarch32 {

/*
  this pass will combine shift instructions into other instructions
*/
class ShiftCombiningPass : public PassInterface {
 public:
  ShiftCombiningPass() {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    // log & apply shift operands information
    for (const auto &i : insts) {
      if (i->IsLabel() || i->IsCall()) {
        Reset();
      }
      else {
        auto inst = static_cast<AArch32Inst *>(i.get());
        switch (inst->opcode()) {
          case OpCode::LSL: case OpCode::LSR: case OpCode::ASR: {
            RemoveDef(inst->dest());
            RemoveUsedByDef(inst->dest());
            LogShiftInfo(inst);
            break;
          }
          case OpCode::ADD: case OpCode::AND: case OpCode::ORR:
          case OpCode::EOR: {
            assert(inst->oprs().size() == 2);
            if (!ApplyShiftInfo(inst)) {
              // swap operands and try again
              auto temp = inst->oprs()[0].value();
              inst->oprs()[0].set_value(inst->oprs()[1].value());
              inst->oprs()[1].set_value(temp);
              ApplyShiftInfo(inst);
            }
            RemoveDef(inst->dest());
            RemoveUsedByDef(inst->dest());
            break;
          }
          case OpCode::SUB: case OpCode::SUBS: case OpCode::RSB:
          case OpCode::CMP: case OpCode::MVN: case OpCode::MOVEQ: {
            ApplyShiftInfo(inst);
            RemoveDef(inst->dest());
            RemoveUsedByDef(inst->dest());
            break;
          }
          default: {
            if (inst->dest() && inst->dest()->IsVirtual()) {
              RemoveDef(inst->dest());
              RemoveUsedByDef(inst->dest());
            }
            break;
          }
        }
      }
    }
    // remove dead instructions
    for (auto it = insts.begin(); it != insts.end();) {
      const auto &dest = (*it)->dest();
      if (dest && dest->IsVirtual() && !dest->use_count()) {
        it = insts.erase(it);
      }
      else {
        ++it;
      }
    }
  }

 private:
  using OpCode = AArch32Inst::OpCode;
  using ShiftOp = AArch32Inst::ShiftOp;

  struct ShiftInfo {
    OprPtr base;
    ShiftOp op;
    std::uint8_t amt;
  };

  void Reset() {
    defs_.clear();
    uses_.clear();
  }

  void RemoveDef(const OprPtr &dest) {
    auto it = defs_.find(dest);
    if (it != defs_.end()) {
      // remove from uses map
      auto [cur, end] = uses_.equal_range(it->second.base);
      for (; cur != end; ++cur) {
        if (cur->second == dest) {
          uses_.erase(cur);
          break;
        }
      }
      // remove from definitions
      defs_.erase(it);
    }
  }

  void RemoveUsedByDef(const OprPtr &opr) {
    auto [begin, end] = uses_.equal_range(opr);
    // remove all definitions with specific value
    for (auto it = begin; it != end; ++it) {
      defs_.erase(it->second);
    }
    // remove expired uses
    uses_.erase(begin, end);
  }

  void LogShiftInfo(AArch32Inst *shift) {
    const auto &dest = shift->dest();
    const auto &base = shift->oprs()[0].value();
    const auto &imm = shift->oprs()[1].value();
    if (dest == base) return;
    // check & get immediate value (shift amount)
    if (!imm->IsImm()) return;
    auto imm_ptr = static_cast<AArch32Imm *>(imm.get());
    auto amt = static_cast<std::uint8_t>(imm_ptr->val());
    // get shift operator
    ShiftOp op;
    switch (shift->opcode()) {
      case OpCode::LSL: op = ShiftOp::LSL; break;
      case OpCode::LSR: op = ShiftOp::LSR; break;
      case OpCode::ASR: op = ShiftOp::ASR; break;
      default: assert(false);
    }
    // update shift info
    defs_.insert({dest, {base, op, amt}});
    uses_.insert({base, dest});
  }

  bool ApplyShiftInfo(AArch32Inst *inst) {
    auto &opr = inst->oprs().back();
    auto it = defs_.find(opr.value());
    if (it == defs_.end()) return false;
    // replace with base register
    opr.set_value(it->second.base);
    // update instruction's shift info
    inst->set_shift_op_amt(it->second.op, it->second.amt);
    return true;
  }

  // all shift info definitions (VReg -> ShiftInfo)
  std::unordered_map<OprPtr, ShiftInfo> defs_;
  // values used by definitions
  std::unordered_multimap<OprPtr, OprPtr> uses_;
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_SHIFTCOMB_H_
