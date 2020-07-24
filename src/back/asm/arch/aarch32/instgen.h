#ifndef BACK_ASM_ARCH_AARCH32_INSTGEN_H_
#define BACK_ASM_ARCH_AARCH32_INSTGEN_H_

#include <unordered_map>
#include <utility>
#include <cassert>

#include "back/asm/arch/instgen.h"
#include "back/asm/arch/aarch32/instdef.h"
#include "back/asm/mir/virtreg.h"
#include "back/asm/mir/label.h"

namespace mimic::back::asmgen::aarch32 {

class AArch32InstGen : public InstGenBase {
 public:
  AArch32InstGen() { Reset(); }

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

 private:
  // get a register
  const OprPtr &GetReg(AArch32Reg::RegName name) {
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
      auto imm = std::make_shared<AArch32Imm>(val);
      auto [it, _] = imms_.insert({val, std::move(imm)});
      return it->second;
    }
  }

  // get a stack slot
  const OprPtr &GetSlot(std::int32_t offset) {
    auto it = slots_.find(offset);
    if (it != slots_.end()) {
      return it->second;
    }
    else {
      auto slot = std::make_shared<AArch32Slot>(offset);
      auto [it, _] = slots_.insert({offset, std::move(slot)});
      return it->second;
    }
  }

  // push a new instruction to current function
  template <typename... Args>
  void PushInst(AArch32Inst::OpCode opcode, Args &&... args) {
    auto inst = std::make_shared<AArch32Inst>(opcode,
                                              std::forward(args)...);
    AddInst(inst);
  }

  // map for registers
  std::unordered_map<AArch32Reg::RegName, OprPtr> regs_;
  // map for immediate numbers / stack slots
  std::unordered_map<std::int32_t, OprPtr> imms_, slots_;
  // for creating virtual registers
  VirtRegFactory vreg_fact_;
  // for creating labels
  LabelFactory label_fact_;
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // BACK_ASM_ARCH_AARCH32_INSTGEN_H_
