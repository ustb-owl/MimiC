#ifndef MIMIC_DEFINE_AST_H_
#define MIMIC_DEFINE_AST_H_

#include <ostream>
#include <optional>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>

#include "define/type.h"
#include "mid/usedef.h"
#include "front/logger.h"

// forward declarations for visitor pattern
namespace mimic::mid {
class Analyzer;
class Evaluator;
class IRBuilder;
}  // namespace mimic::mid

namespace mimic::define {

// definition of base class of all ASTs
class BaseAST {
 public:
  virtual ~BaseAST() = default;

  // dump the content of AST (XML format) to output stream
  virtual void Dump(std::ostream &os) const = 0;
  // run sematic analysis on current AST
  // virtual TypePtr SemaAnalyze(mid::Analyzer &ana) = 0;
  // // evaluate AST (if possible)
  // virtual std::optional<std::uint32_t> Eval(mid::Evaluator &eval) = 0;
  // // generate IR by current AST
  // virtual mid::SSAPtr GenerateIR(mid::IRBuilder &irb) = 0;

  // setters
  void set_logger(const front::Logger &logger) { logger_ = logger; }
  const TypePtr &set_ast_type(const TypePtr &ast_type) {
    return ast_type_ = ast_type;
  }

  // getters
  const front::Logger &logger() const { return logger_; }
  const TypePtr &ast_type() const { return ast_type_; }

 private:
  front::Logger logger_;
  TypePtr ast_type_;
};

using ASTPtr = std::unique_ptr<BaseAST>;
using ASTPtrList = std::vector<ASTPtr>;


// variable/constant declaration
class VarDeclAST : public BaseAST {
 public:
  VarDeclAST(ASTPtr type, ASTPtrList defs)
      : type_(std::move(type)), defs_(std::move(defs)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  ASTPtr type_;
  ASTPtrList defs_;
};

// variable/constant definition
class VarDefAST : public BaseAST {
 public:
  VarDefAST(const std::string &id, ASTPtrList arr_lens, ASTPtr init)
      : id_(id), arr_lens_(std::move(arr_lens)), init_(std::move(init)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  std::string id_;
  ASTPtrList arr_lens_;
  ASTPtr init_;
};

// initializer list (for array initialization)
class InitListAST : public BaseAST {
 public:
  InitListAST(ASTPtrList exprs) : exprs_(std::move(exprs)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  ASTPtrList exprs_;
};

// function declaration
class FuncDeclAST : public BaseAST {
 public:
  FuncDeclAST(ASTPtr type, const std::string &id, ASTPtrList params)
      : type_(std::move(type)), id_(id), params_(std::move(params)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  ASTPtr type_;
  std::string id_;
  ASTPtrList params_;
};

// function definition
class FuncDefAST : public BaseAST {
 public:
  FuncDefAST(ASTPtr header, ASTPtr body)
      : header_(std::move(header)), body_(std::move(body)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  ASTPtr header_, body_;
};

// function parameter
// NOTE: if parameter is an array, 'arr_lens_' must not be empty
//       but it's first element can be 'nullptr' (e.g. int arg[])
class FuncParamAST : public BaseAST {
 public:
  FuncParamAST(ASTPtr type, const std::string &id, ASTPtrList arr_lens)
      : type_(std::move(type)), id_(id), arr_lens_(std::move(arr_lens)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  ASTPtr type_;
  std::string id_;
  ASTPtrList arr_lens_;
};

// structure definition
class StructDefAST : public BaseAST {
 public:
  StructDefAST(const std::string &id, ASTPtrList elems)
      : id_(id), elems_(std::move(elems)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  std::string id_;
  ASTPtrList elems_;
};

// enumeration definition
// NOTE: property 'id' can be empty
class EnumDefAST : public BaseAST {
 public:
  EnumDefAST(const std::string &id, ASTPtrList elems)
      : id_(id), elems_(std::move(elems)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  std::string id_;
  ASTPtrList elems_;
};

// type alias
class TypeAliasAST : public BaseAST {
 public:
  TypeAliasAST(ASTPtr type, const std::string &id)
      : type_(std::move(type)), id_(id) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  ASTPtr type_;
  std::string id_;
};

// element of structure
class StructElemAST : public BaseAST {
 public:
  StructElemAST(ASTPtr type, ASTPtrList defs)
      : type_(std::move(type)), defs_(std::move(defs)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  ASTPtr type_;
  ASTPtrList defs_;
};

// element definition of structure
class StructElemDefAST : public BaseAST {
 public:
  StructElemDefAST(const std::string &id, ASTPtrList arr_lens)
      : id_(id), arr_lens_(std::move(arr_lens)) {}

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  std::string id_;
  ASTPtrList arr_lens_;
};

// element of enumeration
class EnumElemAST : public BaseAST {
 public:
  EnumElemAST(const std::string &id, ASTPtr expr)
      : id_(id), expr_(std::move(expr)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  std::string id_;
  ASTPtr expr_;
};

// statement block
class BlockAST : public BaseAST {
 public:
  BlockAST(ASTPtrList stmts) : stmts_(std::move(stmts)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  ASTPtrList stmts_;
};

// if-else statement
class IfElseAST : public BaseAST {
 public:
  IfElseAST(ASTPtr cond, ASTPtr then, ASTPtr else_then)
      : cond_(std::move(cond)), then_(std::move(then)),
        else_then_(std::move(else_then)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  ASTPtr cond_, then_, else_then_;
};

// while statement
class WhileAST : public BaseAST {
 public:
  WhileAST(ASTPtr cond, ASTPtr body)
      : cond_(std::move(cond)), body_(std::move(body)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  ASTPtr cond_, body_;
};

// control statement
class ControlAST : public BaseAST {
 public:
  enum class Type { Break, Continue, Return };

  ControlAST(Type type, ASTPtr expr)
      : type_(type), expr_(std::move(expr)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  Type type_;
  ASTPtr expr_;
};

// binary expression
class BinaryAST : public BaseAST {
 public:
  enum class Operator {
    Add, Sub, Mul, Div, Mod,
    And, Or, Xor, Shl, Shr,
    LAnd, LOr,
    Equal, NotEqual, Less, LessEq, Great, GreatEq,
    Assign,
    AssAdd, AssSub, AssMul, AssDiv, AssMod,
    AssAnd, AssOr, AssXor, AssShl, AssShr,
  };

  BinaryAST(Operator op, ASTPtr lhs, ASTPtr rhs)
      : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  Operator op_;
  ASTPtr lhs_, rhs_;
};

// type casting expression
class CastAST : public BaseAST {
 public:
  CastAST(ASTPtr type, ASTPtr expr)
      : type_(std::move(type)), expr_(std::move(expr)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  ASTPtr type_, expr_;
};

// unary expression
class UnaryAST : public BaseAST {
 public:
  enum class Operator {
    Pos, Neg, Not, LNot, Deref, Addr, SizeOf,
  };

  UnaryAST(Operator op, ASTPtr opr)
      : op_(op), opr_(std::move(opr)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  Operator op_;
  ASTPtr opr_;
};

// indexing
class IndexAST : public BaseAST {
 public:
  IndexAST(ASTPtr expr, ASTPtr index)
      : expr_(std::move(expr)), index_(std::move(index)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  ASTPtr expr_, index_;
};

// function call
class FuncCallAST : public BaseAST {
 public:
  FuncCallAST(ASTPtr expr, ASTPtrList args)
      : expr_(std::move(expr)), args_(std::move(args)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  ASTPtr expr_;
  ASTPtrList args_;
};

// accessing
// NOTE: property 'is_arrow' can distinguish the accessing opration
//       between '->' (arrow type) and '.' (dot type)
class AccessAST : public BaseAST {
 public:
  AccessAST(bool is_arrow, ASTPtr expr, const std::string &id)
      : is_arrow_(is_arrow), expr_(std::move(expr)), id_(id) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  bool is_arrow_;
  ASTPtr expr_;
  std::string id_;
};

// integer number literal
class IntAST : public BaseAST {
 public:
  IntAST(std::uint32_t value) : value_(value) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  std::uint32_t value_;
};

// character literal
class CharAST : public BaseAST {
 public:
  CharAST(std::uint8_t c) : c_(c) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  std::uint8_t c_;
};

// string literal
class StringAST : public BaseAST {
 public:
  StringAST(const std::string &str) : str_(str) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  std::string str_;
};

// identifier
class IdAST : public BaseAST {
 public:
  IdAST(const std::string &id) : id_(id) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  std::string id_;
};

// primitive type
class PrimTypeAST : public BaseAST {
 public:
  using Type = PrimType::Type;

  PrimTypeAST(Type type) : type_(type) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  Type type_;
};

// structure type
class StructTypeAST : public BaseAST {
 public:
  StructTypeAST(const std::string &id) : id_(id) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  std::string id_;
};

// enumeration type
class EnumTypeAST : public BaseAST {
 public:
  EnumTypeAST(const std::string &id) : id_(id) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  std::string id_;
};

// constant type
class ConstTypeAST : public BaseAST {
 public:
  ConstTypeAST(ASTPtr base) : base_(std::move(base)) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  ASTPtr base_;
};

// pointer type
// NOTE: property 'depth' means count of '*' symbol
class PointerTypeAST : public BaseAST {
 public:
  PointerTypeAST(ASTPtr base, std::size_t depth)
      : base_(std::move(base)), depth_(depth) {}

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  ASTPtr base_;
  std::size_t depth_;
};

// user defined type (type aliases)
class UserTypeAST : public BaseAST {
 public:
  UserTypeAST(const std::string &id) : id_(id) {}

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

 private:
  std::string id_;
};

}  // namespace mimic::define

#endif  // MIMIC_DEFINE_AST_H_
