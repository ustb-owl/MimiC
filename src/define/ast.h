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

  // return true if current AST is a literal value
  virtual bool IsLiteral() const = 0;

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

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const ASTPtr &type() const { return type_; }
  const ASTPtrList &defs() const { return defs_; }

 private:
  ASTPtr type_;
  ASTPtrList defs_;
};

// variable/constant definition
class VarDefAST : public BaseAST {
 public:
  VarDefAST(const std::string &id, ASTPtrList arr_lens, ASTPtr init)
      : id_(id), arr_lens_(std::move(arr_lens)), init_(std::move(init)) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // setters
  void set_init(ASTPtr init) { init_ = std::move(init); }

  // getters
  const std::string &id() const { return id_; }
  const ASTPtrList &arr_lens() const { return arr_lens_; }
  const ASTPtr &init() const { return init_; }

 private:
  std::string id_;
  ASTPtrList arr_lens_;
  ASTPtr init_;
};

// initializer list (for array initialization)
class InitListAST : public BaseAST {
 public:
  InitListAST(ASTPtrList exprs) : exprs_(std::move(exprs)) {}

  bool IsLiteral() const override {
    for (const auto &i : exprs_) {
      if (!i->IsLiteral()) return false;
    }
    return true;
  }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const ASTPtrList &exprs() const { return exprs_; }

 private:
  ASTPtrList exprs_;
};

// function declaration
class FuncDeclAST : public BaseAST {
 public:
  FuncDeclAST(ASTPtr type, const std::string &id, ASTPtrList params)
      : type_(std::move(type)), id_(id), params_(std::move(params)) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const ASTPtr &type() const { return type_; }
  const std::string &id() const { return id_; }
  const ASTPtrList &params() const { return params_; }

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

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const ASTPtr &header() const { return header_; }
  const ASTPtr &body() const { return body_; }

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

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const ASTPtr &type() const { return type_; }
  const std::string &id() const { return id_; }
  const ASTPtrList &arr_lens() const { return arr_lens_; }

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

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const std::string &id() const { return id_; }
  const ASTPtrList &elems() const { return elems_; }

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

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const std::string &id() const { return id_; }
  const ASTPtrList &elems() const { return elems_; }

 private:
  std::string id_;
  ASTPtrList elems_;
};

// type alias
class TypeAliasAST : public BaseAST {
 public:
  TypeAliasAST(ASTPtr type, const std::string &id)
      : type_(std::move(type)), id_(id) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const ASTPtr &type() const { return type_; }
  const std::string &id() const { return id_; }

 private:
  ASTPtr type_;
  std::string id_;
};

// element of structure
class StructElemAST : public BaseAST {
 public:
  StructElemAST(ASTPtr type, ASTPtrList defs)
      : type_(std::move(type)), defs_(std::move(defs)) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const ASTPtr &type() const { return type_; }
  const ASTPtrList &defs() const { return defs_; }

 private:
  ASTPtr type_;
  ASTPtrList defs_;
};

// element definition of structure
class StructElemDefAST : public BaseAST {
 public:
  StructElemDefAST(const std::string &id, ASTPtrList arr_lens)
      : id_(id), arr_lens_(std::move(arr_lens)) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const std::string &id() const { return id_; }
  const ASTPtrList &arr_lens() const { return arr_lens_; }

 private:
  std::string id_;
  ASTPtrList arr_lens_;
};

// element of enumeration
class EnumElemAST : public BaseAST {
 public:
  EnumElemAST(const std::string &id, ASTPtr expr)
      : id_(id), expr_(std::move(expr)) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // setters
  void set_expr(ASTPtr expr) { expr_ = std::move(expr); }

  // getters
  const std::string &id() const { return id_; }
  const ASTPtr &expr() const { return expr_; }

 private:
  std::string id_;
  ASTPtr expr_;
};

// statement block
class BlockAST : public BaseAST {
 public:
  BlockAST(ASTPtrList stmts) : stmts_(std::move(stmts)) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const ASTPtrList &stmts() const { return stmts_; }

 private:
  ASTPtrList stmts_;
};

// if-else statement
class IfElseAST : public BaseAST {
 public:
  IfElseAST(ASTPtr cond, ASTPtr then, ASTPtr else_then)
      : cond_(std::move(cond)), then_(std::move(then)),
        else_then_(std::move(else_then)) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const ASTPtr &cond() const { return cond_; }
  const ASTPtr &then() const { return then_; }
  const ASTPtr &else_then() const { return else_then_; }

 private:
  ASTPtr cond_, then_, else_then_;
};

// while statement
class WhileAST : public BaseAST {
 public:
  WhileAST(ASTPtr cond, ASTPtr body)
      : cond_(std::move(cond)), body_(std::move(body)) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const ASTPtr &cond() const { return cond_; }
  const ASTPtr &body() const { return body_; }

 private:
  ASTPtr cond_, body_;
};

// control statement
class ControlAST : public BaseAST {
 public:
  enum class Type { Break, Continue, Return };

  ControlAST(Type type, ASTPtr expr)
      : type_(type), expr_(std::move(expr)) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  Type type() const { return type_; }
  const ASTPtr &expr() const { return expr_; }

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

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // setters
  void set_lhs(ASTPtr lhs) { lhs_ = std::move(lhs); }
  void set_rhs(ASTPtr rhs) { rhs_ = std::move(rhs); }

  // getters
  Operator op() const { return op_; }
  const ASTPtr &lhs() const { return lhs_; }
  const ASTPtr &rhs() const { return rhs_; }

 private:
  Operator op_;
  ASTPtr lhs_, rhs_;
};

// type casting expression
class CastAST : public BaseAST {
 public:
  CastAST(ASTPtr type, ASTPtr expr)
      : type_(std::move(type)), expr_(std::move(expr)) {}

  bool IsLiteral() const override { return expr_->IsLiteral(); }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // setters
  void set_expr(ASTPtr expr) { expr_ = std::move(expr); }

  // getters
  const ASTPtr &type() const { return type_; }
  const ASTPtr &expr() const { return expr_; }

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

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // setters
  void set_opr(ASTPtr opr) { opr_ = std::move(opr); }

  // getters
  Operator op() const { return op_; }
  const ASTPtr &opr() const { return opr_; }

 private:
  Operator op_;
  ASTPtr opr_;
};

// indexing
class IndexAST : public BaseAST {
 public:
  IndexAST(ASTPtr expr, ASTPtr index)
      : expr_(std::move(expr)), index_(std::move(index)) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const ASTPtr &expr() const { return expr_; }
  const ASTPtr &index() const { return index_; }

 private:
  ASTPtr expr_, index_;
};

// function call
class FuncCallAST : public BaseAST {
 public:
  FuncCallAST(ASTPtr expr, ASTPtrList args)
      : expr_(std::move(expr)), args_(std::move(args)) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const ASTPtr &expr() const { return expr_; }
  const ASTPtrList &args() const { return args_; }

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

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  bool is_arrow() const { return is_arrow_; }
  const ASTPtr &expr() const { return expr_; }
  const std::string &id() const { return id_; }

 private:
  bool is_arrow_;
  ASTPtr expr_;
  std::string id_;
};

// integer number literal
class IntAST : public BaseAST {
 public:
  IntAST(std::uint32_t value) : value_(value) {}

  bool IsLiteral() const override { return true; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  std::uint32_t value() const { return value_; }

 private:
  std::uint32_t value_;
};

// character literal
class CharAST : public BaseAST {
 public:
  CharAST(std::uint8_t c) : c_(c) {}

  bool IsLiteral() const override { return true; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  std::uint8_t c() const { return c_; }

 private:
  std::uint8_t c_;
};

// string literal
class StringAST : public BaseAST {
 public:
  StringAST(const std::string &str) : str_(str) {}

  bool IsLiteral() const override { return true; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const std::string &str() const { return str_; }

 private:
  std::string str_;
};

// identifier
class IdAST : public BaseAST {
 public:
  IdAST(const std::string &id) : id_(id) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const std::string &id() const { return id_; }

 private:
  std::string id_;
};

// primitive type
class PrimTypeAST : public BaseAST {
 public:
  using Type = PrimType::Type;

  PrimTypeAST(Type type) : type_(type) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  Type type() const { return type_; }

 private:
  Type type_;
};

// structure type
class StructTypeAST : public BaseAST {
 public:
  StructTypeAST(const std::string &id) : id_(id) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const std::string &id() const { return id_; }

 private:
  std::string id_;
};

// enumeration type
class EnumTypeAST : public BaseAST {
 public:
  EnumTypeAST(const std::string &id) : id_(id) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const std::string &id() const { return id_; }

 private:
  std::string id_;
};

// constant type
class ConstTypeAST : public BaseAST {
 public:
  ConstTypeAST(ASTPtr base) : base_(std::move(base)) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const ASTPtr &base() const { return base_; }

 private:
  ASTPtr base_;
};

// pointer type
// NOTE: property 'depth' means count of '*' symbol
class PointerTypeAST : public BaseAST {
 public:
  PointerTypeAST(ASTPtr base, std::size_t depth)
      : base_(std::move(base)), depth_(depth) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  // TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  // std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  // mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const ASTPtr &base() const { return base_; }
  std::size_t depth() const { return depth_; }

 private:
  ASTPtr base_;
  std::size_t depth_;
};

// user defined type (type aliases)
class UserTypeAST : public BaseAST {
 public:
  UserTypeAST(const std::string &id) : id_(id) {}

  bool IsLiteral() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr GenerateIR(mid::IRBuilder &irb) override;

  // getters
  const std::string &id() const { return id_; }

 private:
  std::string id_;
};

}  // namespace mimic::define

#endif  // MIMIC_DEFINE_AST_H_
