#include "back/asm/arch/archinfo.h"

#include "back/asm/arch/aarch32/instgen.h"
#include "back/asm/arch/aarch32/passes/brelim.h"
#include "back/asm/mir/passes/movprop.h"
#include "back/asm/mir/passes/movelim.h"
#include "back/asm/mir/passes/linearscan.h"

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
    using RegName = AArch32Reg::RegName;
    PassPtrList list;
    list.push_back(MakePass<BranchEliminationPass>());
    list.push_back(MakePass<MovePropagationPass>());
    list.push_back(MakePass<MoveEliminatePass>());
    // initialize register allocator
    auto reg_alloc = MakePass<LinearScanRegAllocPass>();
    for (int i = static_cast<int>(RegName::R4);
         i <= static_cast<int>(RegName::R10); ++i) {
      reg_alloc->AddAvaliableReg(
          inst_gen_.GetReg(static_cast<RegName>(i)));
    }
    reg_alloc->set_allocator(inst_gen_.GetSlotAllocator());
    list.push_back(std::move(reg_alloc));
    return list;
  }

 private:
  AArch32InstGen inst_gen_;
};

}  // namespace

// register architecture information
REGISTER_ARCH(AArch32ArchInfo, aarch32);
