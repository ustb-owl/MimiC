#ifndef MIMIC_BACK_GENERATOR_H_
#define MIMIC_BACK_GENERATOR_H_

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <cstddef>
#include <string_view>

#include "back/aarch32/armv7asm.h"
#include "back/aarch32/asmgen.h"
#include "back/aarch32/varalloc.h"
#include "back/codegen.h"

namespace mimic::back::aarch32 {

class ARMCodeGen : public back::CodeGenInterface {
 private:
  /* data */
 public:
  ARMCodeGen() { Reset(); }

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
  ARMv7Reg GetVal(const mid::SSAPtr &ssa);
  // get label from metadata
  const std::string &GetLabel(const mid::SSAPtr &ssa);
  // store value/label to metadata
  void SetVal(mid::Value &ssa, const ARMv7Reg val);
  // get a new temporary variable
  std::string GetNewVar(const char *prefix, bool without_prefix);
  // declare new temporary variable and print to output stream
  std::string DeclVar(mid::Value &ssa);
  // generate end of statement
  void GenEnd(mid::Value &ssa);

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
  std::ostringstream type_, code_;
  // set if is in global variable definition
  bool in_global_var_;
  bool has_dump_global_var_;
  bool ignore_gene_ssa_;
  // depth of 'ConstArraySSA'
  std::size_t arr_depth_;

  // current ssa pointer
  mid::SSAPtr current_ssaptr;

  // assembly code generator of function body
  ARMv7AsmGen asm_gen_;
  // variable allocator
  VarAllocation var_alloc_;
  // last SSAs
  mid::ConstIntSSA *last_int_;
  mid::ConstStrSSA *last_str_;
  mid::ArgRefSSA *last_arg_ref_;
  mid::ConstZeroSSA *last_zero_;
  // id of epilogue label
  std::size_t epilogue_id_;
};

}  // namespace mimic::back::aarch32

#endif  // MIMIC_BACK_GENERATOR_H_