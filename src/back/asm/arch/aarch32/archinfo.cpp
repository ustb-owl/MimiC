#include "back/asm/arch/archinfo.h"

#include "back/asm/arch/aarch32/instgen.h"
#include "back/asm/arch/aarch32/passes/brcomb.h"
#include "back/asm/arch/aarch32/passes/brelim.h"
#include "back/asm/arch/aarch32/passes/leacomb.h"
#include "back/asm/arch/aarch32/passes/leaelim.h"
#include "back/asm/arch/aarch32/passes/lsprop.h"
#include "back/asm/mir/passes/movprop.h"
#include "back/asm/mir/passes/movelim.h"
#include "back/asm/arch/aarch32/passes/divopt.h"
#include "back/asm/arch/aarch32/passes/shiftcomb.h"
#include "back/asm/arch/aarch32/passes/liveness.h"
#include "back/asm/mir/passes/linearscan.h"
#include "back/asm/mir/passes/coloring.h"
#include "back/asm/arch/aarch32/passes/slotspill.h"
#include "back/asm/arch/aarch32/passes/funcdeco.h"
#include "back/asm/arch/aarch32/passes/immnorm.h"
#include "back/asm/mir/passes/movoverride.h"
#include "back/asm/arch/aarch32/passes/instsched.h"

using namespace mimic::back::asmgen;
using namespace mimic::back::asmgen::aarch32;

namespace {

class AArch32ArchInfo : public ArchInfoBase {
 public:
  AArch32ArchInfo() {}

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
      list.push_back(MakePass<DivisionOptimizationPass>(inst_gen_));
      list.push_back(MakePass<MovePropagationPass>());
      list.push_back(MakePass<MoveEliminatePass>());
      list.push_back(MakePass<ShiftCombiningPass>());
    }
    list.push_back(MakePass<ImmNormalizePass>(inst_gen_));
    InitRegAlloc(opt_level, list);
    if (opt_level) {
      list.push_back(MakePass<LeaCombiningPass>(inst_gen_));
    }
    list.push_back(MakePass<LeaEliminationPass>(inst_gen_));
    list.push_back(MakePass<SlotSpillingPass>(inst_gen_));
    list.push_back(MakePass<FuncDecoratePass>(inst_gen_));
    if (opt_level) {
      list.push_back(MakePass<LoadStorePropagationPass>());
      list.push_back(MakePass<MovePropagationPass>(IsAvaliableMove));
      list.push_back(MakePass<MoveOverridingPass>());
      list.push_back(MakePass<InstSchedulingPass>());
    }
    return list;
  }

 private:
  using RegName = AArch32Reg::RegName;
  using LiveAnaPtr = std::unique_ptr<LivenessAnalysisPass>;

  static bool IsAvaliableReg(const OprPtr &opr) {
    if (opr->IsReg() && !opr->IsVirtual()) {
      auto reg = static_cast<AArch32Reg *>(opr.get())->name();
      return static_cast<int>(reg) >= static_cast<int>(RegName::R4) &&
             static_cast<int>(reg) <= static_cast<int>(RegName::R10);
    }
    return false;
  }

  static bool IsAvaliableMove(const InstPtr &mov) {
    return IsAvaliableReg(mov->dest()) &&
           IsAvaliableReg(mov->oprs()[0].value());
  }

  static bool IsTempReg(const OprPtr &opr) {
    if (!opr->IsReg() || opr->IsVirtual()) return false;
    auto name = static_cast<AArch32Reg *>(opr.get())->name();
    return name == RegName::R0 || name == RegName::R1 ||
           name == RegName::R2 || name == RegName::LR;
  }

  static void InitTempRegs() {
    for (int i = static_cast<int>(RegName::R0);
         i <= static_cast<int>(RegName::R2); ++i) {
      const auto &reg = inst_gen_.GetReg(static_cast<RegName>(i));
      temp_regs_.push_back(reg);
      temp_regs_with_lr_.push_back(reg);
    }
    temp_regs_with_lr_.push_back(inst_gen_.GetReg(RegName::LR));
  }

  static void InitRegs() {
    for (int i = static_cast<int>(RegName::R4);
         i <= static_cast<int>(RegName::R10); ++i) {
      regs_.push_back(inst_gen_.GetReg(static_cast<RegName>(i)));
    }
  }

  void InitRegAlloc(std::size_t opt_level, PassPtrList &list) {
    using LIType = LivenessAnalysisPass::LivenessInfoType;
    // create liveness analyzer
    auto li_type = opt_level >= 2 ? LIType::InterferenceGraph
                                  : LIType::LiveIntervals;
    auto la = MakePass<LivenessAnalysisPass>(li_type, IsTempReg, temp_regs_,
                                             temp_regs_with_lr_, regs_);
    // create register allocator
    RegAllocPtr reg_alloc;
    if (opt_level >= 2) {
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

  static AArch32InstGen inst_gen_;
  static RegList temp_regs_, temp_regs_with_lr_, regs_;
};

}  // namespace

// register architecture information
REGISTER_ARCH(AArch32ArchInfo, aarch32);


// definitions of static class members
AArch32InstGen AArch32ArchInfo::inst_gen_;
RegList AArch32ArchInfo::temp_regs_;
RegList AArch32ArchInfo::temp_regs_with_lr_;
RegList AArch32ArchInfo::regs_;
