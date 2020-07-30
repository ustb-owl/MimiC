#include "back/asm/generator.h"

using namespace mimic::mid;
using namespace mimic::back::asmgen;

namespace {

void SetOpr(Value &ssa, const OprPtr &opr) {
  if (!ssa.metadata().has_value()) ssa.set_metadata(opr);
}

}  // namespace

void AsmCodeGen::GenerateOn(LoadSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(StoreSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(AccessSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(BinarySSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(UnarySSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(CastSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(CallSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(BranchSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(JumpSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(ReturnSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(FunctionSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(GlobalVarSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(AllocaSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(BlockSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(ArgRefSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(ConstIntSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(ConstStrSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(ConstStructSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(ConstArraySSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(ConstZeroSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(SelectSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(UndefSSA &ssa) {
  SetOpr(ssa, arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::Dump(std::ostream &os) const {
  auto &inst_gen = arch_info_->GetInstGen();
  // run passes
  auto passes = arch_info_->GetPassList(opt_level_);
  for (const auto &pass : passes) {
    inst_gen.RunPass(pass);
  }
  // dump instructions
  inst_gen.Dump(os);
}

bool AsmCodeGen::SetTargetArch(std::string_view arch_name) {
  arch_info_ = ArchManager::GetArch(arch_name);
  if (!arch_info_) {
    return false;
  }
  else {
    arch_info_->GetInstGen().set_parent(this);
    return true;
  }
}
