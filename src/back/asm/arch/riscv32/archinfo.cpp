#include "back/asm/arch/archinfo.h"

#include "back/asm/arch/riscv32/instgen.h"
#include "back/asm/arch/riscv32/passes/brcomb.h"
#include "back/asm/arch/riscv32/passes/brelim.h"
#include "back/asm/arch/riscv32/passes/leacomb.h"
#include "back/asm/arch/riscv32/passes/lsprop.h"
#include "back/asm/mir/passes/movprop.h"
#include "back/asm/mir/passes/movelim.h"
#include "back/asm/arch/riscv32/passes/liveness.h"
#include "back/asm/mir/passes/linearscan.h"
#include "back/asm/mir/passes/coloring.h"
#include "back/asm/arch/riscv32/passes/leaelim.h"
#include "back/asm/arch/riscv32/passes/slotspill.h"
#include "back/asm/arch/riscv32/passes/funcdeco.h"
#include "back/asm/arch/riscv32/passes/immconv.h"
#include "back/asm/arch/riscv32/passes/immnorm.h"
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
    list.push_back(MakePass<BranchCombiningPass>(inst_gen_));
    list.push_back(MakePass<BranchEliminationPass>());
    if (opt_level) {
      list.push_back(MakePass<LeaCombiningPass>(inst_gen_));
      list.push_back(MakePass<LoadStorePropagationPass>());
      list.push_back(MakePass<MovePropagationPass>());
      list.push_back(MakePass<MoveEliminatePass>());
    }
    InitRegAlloc(opt_level, list);
    if (opt_level) {
      list.push_back(MakePass<LeaCombiningPass>(inst_gen_));
    }
    list.push_back(MakePass<LeaEliminationPass>(inst_gen_));
    list.push_back(MakePass<SlotSpillingPass>(inst_gen_));
    list.push_back(MakePass<FuncDecoratePass>(inst_gen_));
    list.push_back(MakePass<ImmConversionPass>(inst_gen_));
    list.push_back(MakePass<ImmNormalizePass>(inst_gen_));
    if (opt_level) {
      list.push_back(MakePass<LoadStorePropagationPass>());
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
      // x9 (without frame pointer), x18-x27
      return id == 9 || (id >= 18 && id <= 27);
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
    // x1, x5-x7, x10-x15 (a6, a7 for compiler), x28-x31
    return id == 1 || (id >= 5 && id <= 7) || (id >= 10 && id <= 15) ||
           (id >= 28 && id <= 31);
  }

  static void InitTempRegs() {
    ADD_TEMP_REGS(5, 7);
    ADD_TEMP_REGS(10, 15);
    ADD_TEMP_REGS(28, 31);
    temp_regs_with_ra_.push_back(inst_gen_.GetReg(RegName::RA));
  }

  static void InitRegs() {
    regs_.push_back(inst_gen_.GetReg(RegName::S1));
    for (int i = 18; i <= 27; ++i) {
      const auto &reg = inst_gen_.GetReg(static_cast<RegName>(i));
      regs_.push_back(reg);
    }
  }

  void InitRegAlloc(std::size_t opt_level, PassPtrList &list) {
    using LIType = LivenessAnalysisPass::LivenessInfoType;
    bool use_gc = opt_level >= 2;
    // create liveness analyzer
    auto li_type = use_gc ? LIType::InterferenceGraph
                          : LIType::LiveIntervals;
    auto la = MakePass<LivenessAnalysisPass>(li_type, IsTempReg, temp_regs_,
                                             temp_regs_with_ra_, regs_);
    // create register allocator
    RegAllocPtr reg_alloc;
    if (use_gc) {
      const auto &fig = la->func_if_graphs();
      auto gcra = MakePass<GraphColoringRegAllocPass>(fig);
      reg_alloc = std::move(gcra);
    }
    else {
      const auto &fli = la->func_live_intervals();
      auto lsra = MakePass<LinearScanRegAllocPass>(fli);
      reg_alloc = std::move(lsra);
    }
    // initialize register lists
    InitTempRegs();
    InitRegs();
    // initialize register allocator
    reg_alloc->set_func_temp_reg_list(&la->func_temp_regs());
    reg_alloc->set_func_reg_list(&la->func_regs());
    reg_alloc->set_allocator(inst_gen_.GetSlotAllocator());
    reg_alloc->set_temp_checker(IsTempReg);
    // add to pass list
    list.push_back(std::move(la));
    list.push_back(std::move(reg_alloc));
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
