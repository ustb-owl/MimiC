#ifndef MIMIC_MID_ANALYZER_H_
#define MIMIC_MID_ANALYZER_H_

#include "mid/eval.h"
#include "define/ast.h"
#include "define/type.h"

namespace mimic::mid {

// perform semantic analysis
class Analyzer {
 public:
  Analyzer(Evaluator &eval) : eval_(eval) {}

  define::TypePtr AnalyzeOn(define::VarDeclAST &ast);
  define::TypePtr AnalyzeOn(define::VarDefAST &ast);
  define::TypePtr AnalyzeOn(define::InitListAST &ast);
  define::TypePtr AnalyzeOn(define::FuncDeclAST &ast);
  define::TypePtr AnalyzeOn(define::FuncDefAST &ast);
  define::TypePtr AnalyzeOn(define::FuncParamAST &ast);
  define::TypePtr AnalyzeOn(define::StructDefAST &ast);
  define::TypePtr AnalyzeOn(define::EnumDefAST &ast);
  define::TypePtr AnalyzeOn(define::TypeAliasAST &ast);
  define::TypePtr AnalyzeOn(define::StructElemAST &ast);
  define::TypePtr AnalyzeOn(define::EnumElemAST &ast);
  define::TypePtr AnalyzeOn(define::BlockAST &ast);
  define::TypePtr AnalyzeOn(define::IfElseAST &ast);
  define::TypePtr AnalyzeOn(define::WhileAST &ast);
  define::TypePtr AnalyzeOn(define::ControlAST &ast);
  define::TypePtr AnalyzeOn(define::BinaryAST &ast);
  define::TypePtr AnalyzeOn(define::CastAST &ast);
  define::TypePtr AnalyzeOn(define::UnaryAST &ast);
  define::TypePtr AnalyzeOn(define::IndexAST &ast);
  define::TypePtr AnalyzeOn(define::FuncCallAST &ast);
  define::TypePtr AnalyzeOn(define::AccessAST &ast);
  define::TypePtr AnalyzeOn(define::IntAST &ast);
  define::TypePtr AnalyzeOn(define::CharAST &ast);
  define::TypePtr AnalyzeOn(define::StringAST &ast);
  define::TypePtr AnalyzeOn(define::IdAST &ast);
  define::TypePtr AnalyzeOn(define::PrimTypeAST &ast);
  define::TypePtr AnalyzeOn(define::StructTypeAST &ast);
  define::TypePtr AnalyzeOn(define::EnumTypeAST &ast);
  define::TypePtr AnalyzeOn(define::ConstTypeAST &ast);
  define::TypePtr AnalyzeOn(define::PointerTypeAST &ast);

 private:
  // evaluator
  Evaluator &eval_;
  //
};

}  // namespace mimic::mid

#endif  // MIMIC_MID_ANALYZER_H_