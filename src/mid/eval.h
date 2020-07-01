#ifndef MIMIC_MID_EVAL_H_
#define MIMIC_MID_EVAL_H_

#include <optional>
#include <cstdint>

#include "define/ast.h"

namespace mimic::mid {

// perform compile time evaluation
class Evaluator {
 public:
  Evaluator() {}

  std::optional<std::uint32_t> EvalOn(define::VarDeclAST &ast);
  std::optional<std::uint32_t> EvalOn(define::VarDefAST &ast);
  std::optional<std::uint32_t> EvalOn(define::InitListAST &ast);
  std::optional<std::uint32_t> EvalOn(define::FuncDeclAST &ast);
  std::optional<std::uint32_t> EvalOn(define::FuncDefAST &ast);
  std::optional<std::uint32_t> EvalOn(define::FuncParamAST &ast);
  std::optional<std::uint32_t> EvalOn(define::StructDefAST &ast);
  std::optional<std::uint32_t> EvalOn(define::EnumDefAST &ast);
  std::optional<std::uint32_t> EvalOn(define::TypeAliasAST &ast);
  std::optional<std::uint32_t> EvalOn(define::StructElemAST &ast);
  std::optional<std::uint32_t> EvalOn(define::EnumElemAST &ast);
  std::optional<std::uint32_t> EvalOn(define::BlockAST &ast);
  std::optional<std::uint32_t> EvalOn(define::IfElseAST &ast);
  std::optional<std::uint32_t> EvalOn(define::WhileAST &ast);
  std::optional<std::uint32_t> EvalOn(define::ControlAST &ast);
  std::optional<std::uint32_t> EvalOn(define::BinaryAST &ast);
  std::optional<std::uint32_t> EvalOn(define::CastAST &ast);
  std::optional<std::uint32_t> EvalOn(define::UnaryAST &ast);
  std::optional<std::uint32_t> EvalOn(define::IndexAST &ast);
  std::optional<std::uint32_t> EvalOn(define::FuncCallAST &ast);
  std::optional<std::uint32_t> EvalOn(define::AccessAST &ast);
  std::optional<std::uint32_t> EvalOn(define::IntAST &ast);
  std::optional<std::uint32_t> EvalOn(define::CharAST &ast);
  std::optional<std::uint32_t> EvalOn(define::StringAST &ast);
  std::optional<std::uint32_t> EvalOn(define::IdAST &ast);
  std::optional<std::uint32_t> EvalOn(define::PrimTypeAST &ast);
  std::optional<std::uint32_t> EvalOn(define::StructTypeAST &ast);
  std::optional<std::uint32_t> EvalOn(define::EnumTypeAST &ast);
  std::optional<std::uint32_t> EvalOn(define::ConstTypeAST &ast);
  std::optional<std::uint32_t> EvalOn(define::PointerTypeAST &ast);

 private:
  //
};

}  // namespace mimic::mid

#endif  // MIMIC_MID_EVAL_H_
