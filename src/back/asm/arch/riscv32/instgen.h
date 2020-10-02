#ifndef MIMIC_BACK_ASM_ARCH_RISCV32_INSTGEN_H_
#define MIMIC_BACK_ASM_ARCH_RISCV32_INSTGEN_H_

#include <unordered_map>
#include <utility>
#include <cassert>

#include "back/asm/arch/instgen.h"
#include "back/asm/arch/riscv32/instdef.h"
#include "back/asm/mir/passes/regalloc.h"
#include "back/asm/mir/virtreg.h"
#include "back/asm/mir/label.h"
#include "utils/hashing.h"

namespace mimic::back::asmgen::riscv32 {

class RISCV32InstGen : public InstGenBase {
 public:
  RISCV32InstGen() { Reset(); }

  OprPtr GenerateOn(mid::LoadSSA &ssa) override;
  OprPtr GenerateOn(mid::StoreSSA &ssa) override;
  OprPtr GenerateOn(mid::AccessSSA &ssa) override;
  OprPtr GenerateOn(mid::BinarySSA &ssa) override;
  OprPtr GenerateOn(mid::UnarySSA &ssa) override;
  OprPtr GenerateOn(mid::CastSSA &ssa) override;
  OprPtr GenerateOn(mid::CallSSA &ssa) override;
  OprPtr GenerateOn(mid::BranchSSA &ssa) override;
  OprPtr GenerateOn(mid::JumpSSA &ssa) override;
  OprPtr GenerateOn(mid::ReturnSSA &ssa) override;
  OprPtr GenerateOn(mid::FunctionSSA &ssa) override;
  OprPtr GenerateOn(mid::GlobalVarSSA &ssa) override;
  OprPtr GenerateOn(mid::AllocaSSA &ssa) override;
  OprPtr GenerateOn(mid::BlockSSA &ssa) override;
  OprPtr GenerateOn(mid::ArgRefSSA &ssa) override;
  OprPtr GenerateOn(mid::ConstIntSSA &ssa) override;
  OprPtr GenerateOn(mid::ConstStrSSA &ssa) override;
  OprPtr GenerateOn(mid::ConstStructSSA &ssa) override;
  OprPtr GenerateOn(mid::ConstArraySSA &ssa) override;
  OprPtr GenerateOn(mid::ConstZeroSSA &ssa) override;
  OprPtr GenerateOn(mid::SelectSSA &ssa) override;
  OprPtr GenerateOn(mid::UndefSSA &ssa) override;

  void Dump(std::ostream &os) const override;

  // reset internal status
  void Reset();
  // get stack slot allocator for register allocation
  SlotAllocator GetSlotAllocator();

  // get an architecture register
  const OprPtr &GetReg(RISCV32Reg::RegName name) const {
    auto it = regs_.find(name);
    assert(it != regs_.end());
    return it->second;
  }

  // get an immediate number
  const OprPtr &GetImm(std::int32_t val) {
    auto it = imms_.find(val);
    if (it != imms_.end()) {
      return it->second;
    }
    else {
      auto imm = std::make_shared<RISCV32Imm>(val);
      return imms_.insert({val, std::move(imm)}).first->second;
    }
  }

  // get a slot
  const OprPtr &GetSlot(const OprPtr &base, std::int32_t offset) {
    assert(base->IsReg() && !base->IsVirtual());
    auto it = slots_.find({base, offset});
    if (it != slots_.end()) {
      return it->second;
    }
    else {
      auto slot = std::make_shared<RISCV32Slot>(base, offset);
      return slots_.insert({{base, offset}, std::move(slot)}).first->second;
    }
  }

  // get a stack slot
  const OprPtr &GetSlot(bool based_on_sp, std::int32_t offset) {
    using RegName = RISCV32Reg::RegName;
    auto reg_name = based_on_sp ? RegName::SP : RegName::FP;
    return GetSlot(GetReg(reg_name), offset);
  }

  // get a in-frame stack slot
  const OprPtr &GetSlot(std::int32_t offset) {
    return GetSlot(false, offset);
  }

  // get a virtual register
  OprPtr GetVReg() { return vreg_fact_.GetReg(); }

  // getters
  // size of all allocated negative-offset in-frame slots
  const std::unordered_map<OprPtr, std::size_t> &alloc_slots() const {
    return alloc_slots_;
  }

 private:
  // allocate next in-frame stack slot
  const OprPtr &AllocNextSlot(const OprPtr &func_label, std::size_t size) {
    std::int32_t ofs = alloc_slots_[func_label] += (size + 3) / 4 * 4;
    return GetSlot(-ofs);
  }

  // push a new instruction to current function
  template <typename... Args>
  std::shared_ptr<RISCV32Inst> PushInst(RISCV32Inst::OpCode opcode,
                                        Args &&... args) {
    auto inst = std::make_shared<RISCV32Inst>(opcode,
                                              std::forward<Args>(args)...);
    AddInst(inst);
    return inst;
  }

  // linkage type conversion
  LinkageTypes GetLinkType(mid::LinkageTypes link);
  // generate zeros
  OprPtr GenerateZeros(const define::TypePtr &type);
  // generate 'memcpy'
  void GenerateMemCpy(const OprPtr &dest, const OprPtr &src,
                      std::size_t size);
  // generate 'memset'
  void GenerateMemSet(const OprPtr &dest, std::uint8_t data,
                      std::size_t size);
  // dump instruction sequences
  void DumpSeqs(std::ostream &os, const InstSeqMap &seqs) const;

  // map for registers
  std::unordered_map<RISCV32Reg::RegName, OprPtr> regs_;
  // map for immediate numbers
  std::unordered_map<std::int32_t, OprPtr> imms_;
  // map for slots
  std::unordered_map<std::pair<OprPtr, std::int32_t>, OprPtr> slots_;
  // size of allocated in-frame stack slots (per function)
  std::unordered_map<OprPtr, std::size_t> alloc_slots_;
  // for creating virtual registers
  VirtRegFactory vreg_fact_;
  // for creating labels
  LabelFactory label_fact_;
  // used when generating functions
  std::vector<OprPtr> args_;
  // used when generating global variables
  std::size_t in_global_;
  // used when generating constant arrays
  std::size_t arr_depth_;
};

}  // namespace mimic::back::asmgen::riscv32

#endif  // MIMIC_BACK_ASM_ARCH_RISCV32_INSTGEN_H_
