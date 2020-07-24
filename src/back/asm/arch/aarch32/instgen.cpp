#include "back/asm/arch/aarch32/instgen.h"

#include "opt/passes/helper/blkiter.h"

using namespace mimic::mid;
using namespace mimic::opt;
using namespace mimic::back::asmgen;
using namespace mimic::back::asmgen::aarch32;

namespace {

using RegName = AArch32Reg::RegName;
using OpCode = AArch32Inst::OpCode;

}  // namespace

OprPtr AArch32InstGen::GenerateOn(LoadSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(StoreSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(AccessSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(BinarySSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(UnarySSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(CastSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(CallSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(BranchSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(JumpSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(ReturnSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(FunctionSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(GlobalVarSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(AllocaSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(BlockSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(ArgRefSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(ConstIntSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(ConstStrSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(ConstStructSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(ConstArraySSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(ConstZeroSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(SelectSSA &ssa) {
  //
}

OprPtr AArch32InstGen::GenerateOn(UndefSSA &ssa) {
  //
}

void AArch32InstGen::Dump(std::ostream &os) const {
  //
}

void AArch32InstGen::Reset() {
  // clear all maps
  regs_.clear();
  imms_.clear();
  slots_.clear();
  // initialize all registers
  for (int i = 0; i < 16; ++i) {
    auto name = static_cast<RegName>(i);
    auto reg = std::make_shared<AArch32Reg>(name);
    regs_.insert({name, reg});
  }
}
