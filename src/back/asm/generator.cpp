#include "back/asm/generator.h"

using namespace mimic::mid;
using namespace mimic::back::asmgen;

void AsmCodeGen::GenerateOn(LoadSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(StoreSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(AccessSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(BinarySSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(UnarySSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(CastSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(CallSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(BranchSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(JumpSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(ReturnSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(FunctionSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(GlobalVarSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(AllocaSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(BlockSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(ArgRefSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(ConstIntSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(ConstStrSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(ConstStructSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(ConstArraySSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(ConstZeroSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(SelectSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
}

void AsmCodeGen::GenerateOn(UndefSSA &ssa) {
  ssa.set_metadata(arch_info_->GetInstGen().GenerateOn(ssa));
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
