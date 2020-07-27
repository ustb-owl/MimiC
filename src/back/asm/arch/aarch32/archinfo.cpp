#include "back/asm/arch/archinfo.h"

#include "back/asm/arch/aarch32/instgen.h"
#include "back/asm/arch/aarch32/passes/brelim.h"
#include "back/asm/mir/passes/movelim.h"
#include "back/asm/mir/passes/fastalloc.h"

using namespace mimic::back::asmgen;
using namespace mimic::back::asmgen::aarch32;

namespace {

class AArch32ArchInfo : public ArchInfoBase {
 public:
  AArch32ArchInfo() {}

  InstGenBase &GetInstGen() override {
    return inst_gen_;
  }

  PassPtrList GetPassList(std::size_t opt_level) override {
    using RegName = AArch32Reg::RegName;
    PassPtrList list;
    list.push_back(MakePass<BranchEliminationPass>());
    list.push_back(MakePass<MoveEliminatePass>());
    // initialize register allocator
    auto fast_alloc = MakePass<FastRegAllocPass>();
    for (int i = static_cast<int>(RegName::R4);
         i <= static_cast<int>(RegName::R10); ++i) {
      fast_alloc->AddAvaliableReg(
          inst_gen_.GetReg(static_cast<RegName>(i)));
    }
    fast_alloc->set_slot_alloc(inst_gen_.GetSlotAllocator());
    list.push_back(std::move(fast_alloc));
    list.push_back(MakePass<MoveEliminatePass>());
    return list;
  }

 private:
  AArch32InstGen inst_gen_;
};

}  // namespace

// register architecture information
REGISTER_ARCH(AArch32ArchInfo, aarch32);
