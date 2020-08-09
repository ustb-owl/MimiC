#include "back/asm/arch/archinfo.h"

#include "back/asm/arch/aarch32/instgen.h"
#include "back/asm/arch/aarch32/passes/brelim.h"
#include "back/asm/arch/aarch32/passes/leacomb.h"
#include "back/asm/arch/aarch32/passes/leaelim.h"
#include "back/asm/arch/aarch32/passes/lsprop.h"
#include "back/asm/mir/passes/movprop.h"
#include "back/asm/mir/passes/movelim.h"
#include "back/asm/arch/aarch32/passes/liveness.h"
#include "back/asm/mir/passes/fastalloc.h"
#include "back/asm/mir/passes/linearscan.h"
#include "back/asm/arch/aarch32/passes/slotspill.h"
#include "back/asm/arch/aarch32/passes/funcdeco.h"
#include "back/asm/arch/aarch32/passes/immnorm.h"

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
    list.push_back(MakePass<ImmNormalizePass>(inst_gen_));
    if (opt_level) {
      list.push_back(MakePass<LoadStorePropagationPass>());
    }
    return list;
  }

 private:
  void InitRegAlloc(std::size_t opt_level, PassPtrList &list) {
    using RegName = AArch32Reg::RegName;
    // create register allocator
    std::unique_ptr<RegAllocatorBase> reg_alloc;
    if (opt_level) {
      auto lsra = MakePass<LinearScanRegAllocPass>();
      auto la = MakePass<LivenessAnalysisPass>();
      lsra->set_func_live_intervals(&la->func_live_intervals());
      list.push_back(std::move(la));
      reg_alloc = std::move(lsra);
    }
    else {
      reg_alloc = MakePass<FastRegAllocPass>();
    }
    // initialize register allocator
    for (int i = static_cast<int>(RegName::R4);
         i <= static_cast<int>(RegName::R10); ++i) {
      reg_alloc->AddAvaliableReg(
          inst_gen_.GetReg(static_cast<RegName>(i)));
    }
    reg_alloc->set_allocator(inst_gen_.GetSlotAllocator());
    list.push_back(std::move(reg_alloc));
  }

  AArch32InstGen inst_gen_;
};

}  // namespace

// register architecture information
REGISTER_ARCH(AArch32ArchInfo, aarch32);
