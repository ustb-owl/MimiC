#ifndef MIMIC_MID_IRBUILDER_H_
#define MIMIC_MID_IRBUILDER_H_

#include "define/ast.h"
#include "mid/usedef.h"

namespace mimic::mid {

class IRBuilder {
 public:
  IRBuilder() {}

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

 private:
  //
};

}  // namespace mimic::mid

#endif  // MIMIC_MID_IRBUILDER_H_
