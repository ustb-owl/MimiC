#include "define/ast.h"

#include "mid/analyzer.h"
#include "mid/eval.h"

using namespace mimic::define;
using namespace mimic::mid;


TypePtr VarDeclAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr VarDefAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr InitListAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr FuncDeclAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr FuncDefAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr FuncParamAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr StructDefAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr EnumDefAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr TypeAliasAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr StructElemAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr EnumElemAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr BlockAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr IfElseAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr WhileAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr ControlAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr BinaryAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr CastAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr UnaryAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr IndexAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr FuncCallAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr AccessAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr IntAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr CharAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr StringAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr IdAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr PrimTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr StructTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr EnumTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr ConstTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr PointerTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}


std::optional<std::uint32_t> VarDeclAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> VarDefAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> InitListAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> FuncDeclAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> FuncDefAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> FuncParamAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> StructDefAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> EnumDefAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> TypeAliasAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> StructElemAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> EnumElemAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> BlockAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> IfElseAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> WhileAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> ControlAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> BinaryAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> CastAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> UnaryAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> IndexAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> FuncCallAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> AccessAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> IntAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> CharAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> StringAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> IdAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> PrimTypeAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> StructTypeAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> EnumTypeAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> ConstTypeAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> PointerTypeAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}
