#include "back/asm/arch/archinfo.h"

#include "back/asm/arch/aarch32/instgen.h"

using namespace mimic::back::asmgen;
using namespace mimic::back::asmgen::aarch32;

namespace {

class AArch32ArchInfo : public ArchInfoBase {
 public:
  AArch32ArchInfo() {}

  InstGenBase &GetInstGen() override {
    return inst_gen_;
  }

  PassPtrList GetPassList(std::size_t opt_level) const override {
    return {};
  }

 private:
  AArch32InstGen inst_gen_;
};

}  // namespace

// register architecture information
REGISTER_ARCH(AArch32ArchInfo, aarch32);
