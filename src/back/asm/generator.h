#ifndef MIMIC_BACK_ASM_GENERATOR_H_
#define MIMIC_BACK_ASM_GENERATOR_H_

#include "back/codegen.h"

namespace mimic::back::asmgen {

class AsmCodeGen : public CodeGenInterface {
 public:
  AsmCodeGen() { Reset(); }

  void Reset();

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

 private:
  //
};

}  // namespace mimic::back::asm

#endif  // MIMIC_BACK_ASM_GENERATOR_H_
