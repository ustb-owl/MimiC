#ifndef MIMIC_BACK_ASM_GENERATOR_H_
#define MIMIC_BACK_ASM_GENERATOR_H_

#include <string_view>
#include <cstddef>

#include "back/codegen.h"
#include "back/asm/arch/archinfo.h"

namespace mimic::back::asmgen {

// code generator for multi-architecture assembly
class AsmCodeGen : public CodeGenInterface {
 public:
  AsmCodeGen() : opt_level_(0) {}

  void GenerateOn(mid::LoadSSA &ssa) override;
  void GenerateOn(mid::StoreSSA &ssa) override;
  void GenerateOn(mid::AccessSSA &ssa) override;
  void GenerateOn(mid::BinarySSA &ssa) override;
  void GenerateOn(mid::UnarySSA &ssa) override;
  void GenerateOn(mid::CastSSA &ssa) override;
  void GenerateOn(mid::CallSSA &ssa) override;
  void GenerateOn(mid::BranchSSA &ssa) override;
  void GenerateOn(mid::JumpSSA &ssa) override;
  void GenerateOn(mid::ReturnSSA &ssa) override;
  void GenerateOn(mid::FunctionSSA &ssa) override;
  void GenerateOn(mid::GlobalVarSSA &ssa) override;
  void GenerateOn(mid::AllocaSSA &ssa) override;
  void GenerateOn(mid::BlockSSA &ssa) override;
  void GenerateOn(mid::ArgRefSSA &ssa) override;
  void GenerateOn(mid::ConstIntSSA &ssa) override;
  void GenerateOn(mid::ConstStrSSA &ssa) override;
  void GenerateOn(mid::ConstStructSSA &ssa) override;
  void GenerateOn(mid::ConstArraySSA &ssa) override;
  void GenerateOn(mid::ConstZeroSSA &ssa) override;
  void GenerateOn(mid::SelectSSA &ssa) override;
  void GenerateOn(mid::UndefSSA &ssa) override;

  void Dump(std::ostream &os) const override;

  // set target architecture of assembly generator
  // returns false if target is invalid
  bool SetTargetArch(std::string_view arch_name);
  // display all avaliable architectures
  void ShowAvaliableArchs(std::ostream &os);

  // setters
  void set_opt_level(std::size_t opt_level) { opt_level_ = opt_level; }

 private:
  // info of target architecture
  ArchInfoPtr arch_info_;
  // optimization level
  std::size_t opt_level_;
};

}  // namespace mimic::back::asm

#endif  // MIMIC_BACK_ASM_GENERATOR_H_
