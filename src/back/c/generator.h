#ifndef MIMIC_BACK_C_GENERATOR_H_
#define MIMIC_BACK_C_GENERATOR_H_

#include <utility>
#include <string>
#include <vector>
#include <sstream>
#include <cstddef>

#include "back/codegen.h"
#include "define/type.h"

namespace mimic::back::c {

class CCodeGen : public CodeGenInterface {
 public:
  CCodeGen() { Reset(); }

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
  using TypeInfo = std::pair<define::TypePtr, std::string>;

  // get value from metadata
  const std::string &GetVal(const mid::SSAPtr &ssa);
  // get label from metadata
  const std::string &GetLabel(const mid::SSAPtr &ssa);
  // store value/label to metadata
  void SetVal(mid::Value &ssa, const std::string &val);
  // get a new temporary variable
  std::string GetNewVar(const char *prefix);
  // declare new temporary variable and print to output stream
  std::string DeclVar(mid::Value &ssa);
  // generate end of statement
  void GenEnd(mid::Value &ssa);

  // get C-style type name
  std::string GetTypeName(const define::TypePtr &type);
  // generate the definition of structure type
  const std::string &DefineStruct(const define::TypePtr &type);
  // generate the definition of array type
  const std::string &DefineArray(const define::TypePtr &type);

  // temporary variable/type counter
  std::size_t counter_;
  // structures/arrays that have already been defined
  std::vector<TypeInfo> defed_types_;
  // output stream for generated code
  std::ostringstream type_, global_alloca_, code_;
  // set if is in global variable definition
  bool in_global_var_;
  // depth of 'ConstArraySSA'
  std::size_t arr_depth_;
};

}  // namespace mimic::back::c

#endif  // MIMIC_BACK_C_GENERATOR_H_
