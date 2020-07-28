#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_FUNCDECO_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_FUNCDECO_H_

#include <bitset>
#include <unordered_set>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/aarch32/instdef.h"
#include "back/asm/arch/aarch32/instgen.h"

namespace mimic::back::asmgen::aarch32 {

/*
  this pass will generate prologue/epilogue for functions when necessary
*/
class FuncDecoratePass : public PassInterface {
 public:
  FuncDecoratePass(AArch32InstGen &gen) : gen_(gen) {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    // collect related information, including:
    // 1. usage of all preserved registers (r4-r10)
    // 2. whether there are function calls in current function
    // 3. slots that should be preserved for function calls (arguments)
    // 4. sum of all the sizes of in-frame slots
    // 5. position of return instruction (i.e. 'bx lr')
    Reset();
    for (auto it = insts.begin(); it != insts.end(); ++it) {
      const auto &i = *it;
      if (i->IsCall() && !has_call_) {
        has_call_ = true;
      }
      else {
        if (i->dest()) LogPreservedReg(i->dest());
        auto opcode = static_cast<AArch32Inst *>(i.get())->opcode();
        if (opcode == OpCode::STR) {
          LogSlotInfo(i->oprs()[1].value());
        }
        else if (opcode == OpCode::LDR) {
          LogSlotInfo(i->oprs()[0].value());
        }
        else if (opcode == OpCode::BX && i->oprs()[0].value()->IsReg()) {
          const auto &reg = i->oprs()[0].value();
          auto name = static_cast<AArch32Reg *>(reg.get())->name();
          if (name == RegName::LR) ret_pos_ = it;
        }
      }
    }
    // if there are function calls, 'lr' should be preserved
    if (has_call_) used_regs_ |= 1 << static_cast<int>(RegName::LR);
    // if there are any stack slots, 'r11' should be preserved
    if (preserved_slot_size_ || slot_size_) {
      used_regs_ |= 1 << static_cast<int>(RegName::R11);
    }
    // calculate additional offset of positive-offset in-frame slot
    std::int32_t add_pos_offset = GetUsedRegCount() * 4;
    // get total size of negative-offset slots
    auto neg_slot_size = preserved_slot_size_ + slot_size_;
    // generate push/pop
    if (used_regs_) {
      ret_pos_ = ++insts.insert(ret_pos_, MakePop());
      if (has_call_) insts.erase(ret_pos_);
      insts.insert(insts.begin(), MakePush());
    }
    if (neg_slot_size) {
      // generate instructions for stack pointer update
      UpdateSP(insts, ++insts.begin(), neg_slot_size);
      // update all positive-offset in-frame slots
      for (auto &&i : poif_slots_) {
        i->set_offset(i->offset() + add_pos_offset);
      }
    }
  }

 private:
  using OpCode = AArch32Inst::OpCode;
  using RegName = AArch32Reg::RegName;

  void Reset() {
    used_regs_ = 0;
    has_call_ = false;
    preserved_slot_size_ = 0;
    slot_size_ = 0;
    poif_slots_.clear();
  }

  void LogPreservedReg(const OprPtr &opr) {
    if (!opr->IsReg()) return;
    auto name = static_cast<AArch32Reg *>(opr.get())->name();
    auto name_i = static_cast<int>(name);
    auto l = static_cast<int>(RegName::R4);
    auto u = static_cast<int>(RegName::R10);
    if (name_i >= l && name_i <= u) used_regs_ |= 1 << name_i;
  }

  void LogSlotInfo(const OprPtr &opr) {
    if (!opr->IsSlot()) return;
    auto slot = static_cast<AArch32Slot *>(opr.get());
    if (slot->based_on_sp()) {
      std::size_t ofs = slot->offset() + 4;
      if (ofs > preserved_slot_size_) preserved_slot_size_ = ofs;
    }
    else if (slot->offset() < 0) {
      std::size_t ofs = -slot->offset();
      if (ofs > slot_size_) slot_size_ = ofs;
    }
    else if (slot->offset() >= 0) {
      poif_slots_.insert(slot);
    }
  }

  std::size_t GetUsedRegCount() {
    std::bitset<16> b(used_regs_);
    return b.count();
  }

  InstPtr MakePush() {
    auto inst = std::make_shared<AArch32Inst>(OpCode::PUSH);
    for (std::size_t i = 0; i < 16; ++i) {
      if (used_regs_ & (1 << i)) {
        auto name = static_cast<RegName>(i);
        inst->AddOpr(gen_.GetReg(name));
      }
    }
    return inst;
  }

  InstPtr MakePop() {
    auto inst = std::make_shared<AArch32Inst>(OpCode::POP);
    for (std::size_t i = 0; i < 16; ++i) {
      if (used_regs_ & (1 << i)) {
        auto name = static_cast<RegName>(i);
        if (name == RegName::LR) name = RegName::PC;
        inst->AddOpr(gen_.GetReg(name));
      }
    }
    return inst;
  }

  void UpdateSP(InstPtrList &insts, InstPtrList::iterator pos,
                std::size_t size) {
    const auto &r11 = gen_.GetReg(RegName::R11);
    const auto &sp = gen_.GetReg(RegName::SP);
    // generate 'mov'
    auto mov = std::make_shared<AArch32Inst>(OpCode::MOV, r11, sp);
    pos = ++insts.insert(pos, mov);
    // generate 'sub'
    auto imm = gen_.GetImm(size);
    auto sub = std::make_shared<AArch32Inst>(OpCode::SUB, sp, sp, imm);
    insts.insert(pos, sub);
  }

  AArch32InstGen &gen_;
  // bit mask of all used preserved registers
  std::size_t used_regs_;
  // set if there are function calls
  bool has_call_;
  // preserved slots for arguments
  std::size_t preserved_slot_size_;
  // in-frame slot size
  std::size_t slot_size_;
  // instructions that used positive-offset in-frame slot
  std::unordered_set<AArch32Slot *> poif_slots_;
  // position of return instruction
  InstPtrList::iterator ret_pos_;
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_FUNCDECO_H_
