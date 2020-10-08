#ifndef MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_FUNCDECO_H_
#define MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_FUNCDECO_H_

#include <bitset>
#include <unordered_map>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/riscv32/instdef.h"
#include "back/asm/arch/riscv32/instgen.h"

namespace mimic::back::asmgen::riscv32 {

/*
  this pass will generate prologue/epilogue for functions when necessary
*/
class FuncDecoratePass : public PassInterface {
 public:
  FuncDecoratePass(RISCV32InstGen &gen) : gen_(gen) {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    // collect related information, including:
    // 1. usage of all preserved registers (s1-s11)
    // 2. whether there are function calls in current function
    // 3. slots that should be preserved for function calls (arguments)
    // 4. sum of all the sizes of in-frame slots
    // 5. position of return instruction (i.e. 'ret')
    Reset();
    for (auto it = insts.begin(); it != insts.end(); ++it) {
      const auto &i = *it;
      if (i->IsCall() && !has_call_) {
        has_call_ = true;
      }
      else {
        if (i->dest()) LogPreservedReg(i->dest());
        auto inst = static_cast<RISCV32Inst *>(i.get());
        if (inst->opcode() == OpCode::SW) {
          LogSlotInfo(i->oprs()[1].value(), inst);
        }
        else if (inst->opcode() == OpCode::LW) {
          LogSlotInfo(i->oprs()[0].value(), inst);
        }
        else if (inst->opcode() == OpCode::RET) {
          ret_pos_ = it;
        }
      }
    }
    // if there are function calls, 'ra' should be preserved
    if (has_call_) used_regs_ |= 1 << static_cast<int>(RegName::RA);
    // if there are any stack slots, 'fp' should be preserved
    auto it = gen_.alloc_slots().find(func_label);
    auto slot_size = it != gen_.alloc_slots().end() ? it->second : 0;
    if (preserved_slot_size_ || slot_size) {
      used_regs_ |= 1 << static_cast<int>(RegName::FP);
    }
    // calculate additional offset of positive-offset in-frame slot
    std::int32_t add_pos_offset = GetUsedRegCount() * 4;
    // get total size of negative-offset slots
    auto neg_slot_size = preserved_slot_size_ + slot_size;
    // generate push/pop
    if (used_regs_) {
      MakePop(insts, add_pos_offset, neg_slot_size);
      MakePush(insts, add_pos_offset, neg_slot_size);
    }
    // update all positive-offset in-frame slots
    if (add_pos_offset) {
      for (auto &&[inst, slot] : poif_slots_) {
        auto new_slot = gen_.GetSlot(slot->offset() + add_pos_offset);
        inst->oprs()[inst->opcode() == OpCode::SW].set_value(new_slot);
        slot = static_cast<RISCV32Slot *>(new_slot.get());
      }
    }
    // convert all positive-offset in-frame slots to sp-based slots
    if (!neg_slot_size) {
      for (const auto &[inst, slot] : poif_slots_) {
        assert(slot->base() == gen_.GetReg(RegName::FP));
        auto new_slot = gen_.GetSlot(true, slot->offset());
        inst->oprs()[inst->opcode() == OpCode::SW].set_value(new_slot);
      }
    }
  }

 private:
  using OpCode = RISCV32Inst::OpCode;
  using RegName = RISCV32Reg::RegName;
  using InstIt = InstPtrList::iterator;

  void Reset() {
    used_regs_ = 0;
    has_call_ = false;
    preserved_slot_size_ = 0;
    poif_slots_.clear();
  }

  void LogPreservedReg(const OprPtr &opr) {
    if (!opr->IsReg()) return;
    auto name = static_cast<RISCV32Reg *>(opr.get())->name();
    auto name_i = static_cast<int>(name);
    auto s1 = static_cast<int>(RegName::S1);
    auto l = static_cast<int>(RegName::S2);
    auto u = static_cast<int>(RegName::S11);
    if (name_i == s1 || (name_i >= l && name_i <= u)) {
      used_regs_ |= 1 << name_i;
    }
  }

  void LogSlotInfo(const OprPtr &opr, RISCV32Inst *inst) {
    if (!opr->IsSlot()) return;
    auto slot = static_cast<RISCV32Slot *>(opr.get());
    auto base_name = static_cast<RISCV32Reg *>(slot->base().get())->name();
    if (base_name == RegName::SP) {
      std::size_t ofs = slot->offset() + 4;
      if (ofs > preserved_slot_size_) preserved_slot_size_ = ofs;
    }
    else if (base_name == RegName::FP && slot->offset() >= 0) {
      poif_slots_.insert({inst, slot});
    }
  }

  std::size_t GetUsedRegCount() {
    std::bitset<16> b(used_regs_);
    return b.count();
  }

  template <typename... Args>
  InstIt InsertBefore(InstPtrList &insts, InstIt pos, Args &&... args) {
    auto inst = std::make_shared<RISCV32Inst>(std::forward<Args>(args)...);
    return ++insts.insert(pos, std::move(inst));
  }

  void MakePush(InstPtrList &insts, std::int32_t saved_size,
                std::int32_t slot_size) {
    /*
      old sp -->  +-----------------+   ^
                  |   saved  regs   |   | saved size
      fp     -->  +-----------------+   X
                  | spilled  values |   |
                  +-----------------+   | slot size
                  |  spilled  args  |   |
      new sp -->  +-----------------+   V
    */
    const auto &sp = gen_.GetReg(RegName::SP);
    const auto &fp = gen_.GetReg(RegName::FP);
    auto pos = insts.begin();
    // update stack pointer
    pos = InsertBefore(insts, pos, OpCode::ADDI, sp, sp,
                       gen_.GetImm(-(saved_size + slot_size)));
    // store all saved registers
    auto cur_ofs = saved_size + slot_size - 4;
    for (std::size_t i = 0; i < 32; ++i) {
      if (used_regs_ & (1 << i)) {
        const auto &reg = gen_.GetReg(static_cast<RegName>(i));
        const auto &slot = gen_.GetSlot(true, cur_ofs);
        pos = InsertBefore(insts, pos, OpCode::SW, reg, slot);
        cur_ofs -= 4;
      }
    }
    // update frame pointer
    if (slot_size) {
      pos = InsertBefore(insts, pos, OpCode::ADDI, fp, sp,
                         gen_.GetImm(slot_size));
    }
  }

  void MakePop(InstPtrList &insts, std::int32_t saved_size,
               std::int32_t slot_size) {
    const auto &sp = gen_.GetReg(RegName::SP);
    // restore all saved registers
    auto cur_ofs = saved_size + slot_size - 4;
    for (std::size_t i = 0; i < 16; ++i) {
      if (used_regs_ & (1 << i)) {
        const auto &reg = gen_.GetReg(static_cast<RegName>(i));
        const auto &slot = gen_.GetSlot(true, cur_ofs);
        ret_pos_ = InsertBefore(insts, ret_pos_, OpCode::LW, reg, slot);
        cur_ofs -= 4;
      }
    }
    // update stack pointer
    ret_pos_ = InsertBefore(insts, ret_pos_, OpCode::ADDI, sp, sp,
                            gen_.GetImm(saved_size + slot_size));
  }

  RISCV32InstGen &gen_;
  // bit mask of all used preserved registers
  std::size_t used_regs_;
  // set if there are function calls
  bool has_call_;
  // preserved slots for arguments
  std::size_t preserved_slot_size_;
  // info of all positive-offset in-frame slots
  std::unordered_map<RISCV32Inst *, RISCV32Slot *> poif_slots_;
  // position of return instruction
  InstPtrList::iterator ret_pos_;
};

}  // namespace mimic::back::asmgen::riscv32

#endif  // MIMIC_BACK_ASM_ARCH_RISCV32_PASSES_FUNCDECO_H_
