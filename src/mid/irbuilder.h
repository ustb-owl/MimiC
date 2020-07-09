#ifndef MIMIC_MID_IRBUILDER_H_
#define MIMIC_MID_IRBUILDER_H_

#include <utility>
#include <string>
#include <string_view>
#include <unordered_map>
#include <stack>

#include "define/ast.h"
#include "mid/usedef.h"
#include "mid/module.h"
#include "xstl/guard.h"
#include "xstl/nested.h"

namespace mimic::mid {

class IRBuilder {
 public:
  IRBuilder() { Reset(); }

  void Reset();

  SSAPtr GenerateOn(define::VarDeclAST &ast);
  SSAPtr GenerateOn(define::VarDefAST &ast);
  SSAPtr GenerateOn(define::InitListAST &ast);
  SSAPtr GenerateOn(define::FuncDeclAST &ast);
  SSAPtr GenerateOn(define::FuncDefAST &ast);
  SSAPtr GenerateOn(define::FuncParamAST &ast);
  SSAPtr GenerateOn(define::StructDefAST &ast);
  SSAPtr GenerateOn(define::EnumDefAST &ast);
  SSAPtr GenerateOn(define::TypeAliasAST &ast);
  SSAPtr GenerateOn(define::StructElemAST &ast);
  SSAPtr GenerateOn(define::StructElemDefAST &ast);
  SSAPtr GenerateOn(define::EnumElemAST &ast);
  SSAPtr GenerateOn(define::BlockAST &ast);
  SSAPtr GenerateOn(define::IfElseAST &ast);
  SSAPtr GenerateOn(define::WhileAST &ast);
  SSAPtr GenerateOn(define::ControlAST &ast);
  SSAPtr GenerateOn(define::BinaryAST &ast);
  SSAPtr GenerateOn(define::CastAST &ast);
  SSAPtr GenerateOn(define::UnaryAST &ast);
  SSAPtr GenerateOn(define::IndexAST &ast);
  SSAPtr GenerateOn(define::FuncCallAST &ast);
  SSAPtr GenerateOn(define::AccessAST &ast);
  SSAPtr GenerateOn(define::IntAST &ast);
  SSAPtr GenerateOn(define::CharAST &ast);
  SSAPtr GenerateOn(define::StringAST &ast);
  SSAPtr GenerateOn(define::IdAST &ast);
  SSAPtr GenerateOn(define::PrimTypeAST &ast);
  SSAPtr GenerateOn(define::StructTypeAST &ast);
  SSAPtr GenerateOn(define::EnumTypeAST &ast);
  SSAPtr GenerateOn(define::ConstTypeAST &ast);
  SSAPtr GenerateOn(define::PointerTypeAST &ast);
  SSAPtr GenerateOn(define::UserTypeAST &ast);

  // getters
  Module &module() { return module_; }

 private:
  // pair for storing target block of break & continue
  using BreakCont = std::pair<BlockPtr, BlockPtr>;

  // switch to a new environment
  xstl::Guard NewEnv();
  // create binary operation
  SSAPtr CreateBinOp(define::BinaryAST::Operator op, const SSAPtr &lhs,
                     const SSAPtr &rhs);

  // module for storing IRs
  Module module_;
  // table of values
  xstl::NestedMapPtr<std::string, SSAPtr> vals_;
  // used when generating functions
  bool in_func_;
  std::unordered_map<std::string_view, UserPtr> funcs_;
  SSAPtr ret_val_;
  BlockPtr func_exit_;
  // used when generating loops
  std::stack<BreakCont> break_cont_;
};

}  // namespace mimic::mid

#endif  // MIMIC_MID_IRBUILDER_H_
