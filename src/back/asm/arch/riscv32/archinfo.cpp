#include "back/asm/arch/archinfo.h"

#include "back/asm/arch/riscv32/instgen.h"
#include "back/asm/mir/passes/movprop.h"
#include "back/asm/mir/passes/movelim.h"
#include "back/asm/mir/passes/movoverride.h"

using namespace mimic::back::asmgen;
using namespace mimic::back::asmgen::riscv32;

#define ADD_TEMP_REGS(l, r)                                        \
  do {                                                             \
    for (int i = (l); i <= (r); ++i) {                             \
      const auto &reg = inst_gen_.GetReg(static_cast<RegName>(i)); \
      temp_regs_.push_back(reg);                                   \
      temp_regs_with_ra_.push_back(reg);                           \
    }                                                              \
  } while (0)

#define ADD_REGS(l, r)                                             \
  do {                                                             \
    for (int i = (l); i <= (r); ++i) {                             \
      const auto &reg = inst_gen_.GetReg(static_cast<RegName>(i)); \
      regs_.push_back(reg);                                        \
    }                                                              \
  } while (0)

namespace {

class RISCV32ArchInfo : public ArchInfoBase {
 public:
  RISCV32ArchInfo() {}

  std::size_t GetPtrSize() const override { return 4; }

  InstGenBase &GetInstGen() override {
    return inst_gen_;
  }

  PassPtrList GetPassList(std::size_t opt_level) override {
    PassPtrList list;
    if (opt_level) {
      list.push_back(MakePass<MovePropagationPass>());
      list.push_back(MakePass<MoveEliminatePass>());
      list.push_back(MakePass<MovePropagationPass>(IsAvaliableMove));
      list.push_back(MakePass<MoveOverridingPass>());
    }
    return list;
  }

 private:
  using RegName = RISCV32Reg::RegName;

  static bool IsAvaliableReg(const OprPtr &opr) {
    if (opr->IsReg() && !opr->IsVirtual()) {
      auto name = static_cast<RISCV32Reg *>(opr.get())->name();
      auto id = static_cast<int>(name);
      // x8-x9, x18-x27
      return (id >= 8 && id <= 9) || (id >= 18 && id <= 27);
    }
    return false;
  }

  static bool IsAvaliableMove(const InstPtr &mov) {
    return IsAvaliableReg(mov->dest()) &&
           IsAvaliableReg(mov->oprs()[0].value());
  }

  static bool IsTempReg(const OprPtr &opr) {
    if (!opr->IsReg() || opr->IsVirtual()) return false;
    auto name = static_cast<RISCV32Reg *>(opr.get())->name();
    auto id = static_cast<int>(name);
    // x1, x5-x7, x10-x17, x28-x31
    return id == 1 || (id >= 5 && id <= 7) || (id >= 10 && id <= 17) ||
           (id >= 28 && id <= 31);
  }

  static void InitTempRegs() {
    ADD_TEMP_REGS(5, 7);
    ADD_TEMP_REGS(10, 17);
    ADD_TEMP_REGS(28, 31);
    temp_regs_with_ra_.push_back(inst_gen_.GetReg(RegName::RA));
  }

  static void InitRegs() {
    ADD_REGS(8, 9);
    ADD_REGS(18, 27);
  }

  static RISCV32InstGen inst_gen_;
  static RegList temp_regs_, temp_regs_with_ra_, regs_;
};

}  // namespace

// register architecture information
REGISTER_ARCH(RISCV32ArchInfo, riscv32);


// definitions of static class members
RISCV32InstGen RISCV32ArchInfo::inst_gen_;
RegList RISCV32ArchInfo::temp_regs_;
RegList RISCV32ArchInfo::temp_regs_with_ra_;
RegList RISCV32ArchInfo::regs_;
