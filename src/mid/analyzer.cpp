#include "mid/analyzer.h"

#include <cassert>

using namespace mimic::mid;
using namespace mimic::define;
using namespace mimic::front;

namespace {

// print error message
inline TypePtr LogError(const Logger &log, std::string_view message) {
  log.LogError(message);
  return nullptr;
}

// print error message (with specific identifier)
inline TypePtr LogError(const Logger &log, std::string_view message,
                        std::string_view id) {
  log.LogError(message, id);
  return nullptr;
}

// get common type of two specific types
// perform implicit casting of integer types
inline const TypePtr &GetCommonType(const TypePtr &t1, const TypePtr &t2) {
  assert(t1->IsInteger() && t2->IsInteger());
  if (t1->GetSize() != t2->GetSize()) {
    return t1->GetSize() > t2->GetSize() ? t1 : t2;
  }
  else {
    return t1->IsUnsigned() ? t1 : t2;
  }
}

}  // namespace

// definition of static properties
TypePtr Analyzer::enum_base_ = MakePrimType(PrimType::Type::Int32, true);

xstl::Guard Analyzer::NewEnv() {
  symbols_ = xstl::MakeNestedMap(symbols_);
  aliases_ = xstl::MakeNestedMap(aliases_);
  structs_ = xstl::MakeNestedMap(structs_);
  enums_ = xstl::MakeNestedMap(enums_);
  return xstl::Guard([this] {
    symbols_ = symbols_->outer();
    aliases_ = aliases_->outer();
    structs_ = structs_->outer();
    enums_ = enums_->outer();
  });
}

TypePtr Analyzer::HandleArray(TypePtr base, const ASTPtrList &arr_lens,
                              std::string_view id, bool is_param) {
  for (auto i = arr_lens.size() - 1; i >= 0; --i) {
    const auto &expr = arr_lens[i];
    if (is_param && (!expr || !i)) {
      // check error
      if (!expr && i) {
        return LogError(expr->logger(), "incomplete array type", id);
      }
      // make a pointer
      base = MakePointer(std::move(base), false);
    }
    else {
      // try to evaluate current dimension
      auto len = expr->Eval(eval_);
      if (!len) {
        return LogError(expr->logger(), "invalid array length", id);
      }
      // make array type
      base = std::make_shared<ArrayType>(std::move(base), *len, false);
    }
  }
  return base;
}

TypePtr Analyzer::AnalyzeOn(VarDeclAST &ast) {
  // get type & check
  var_type_ = ast.type()->SemaAnalyze(*this);
  if (!var_type_) return nullptr;
  if (var_type_->IsVoid()) {
    return LogError(ast.type()->logger(), "variable can not be void type");
  }
  // handle definitions
  for (const auto &i : ast.defs()) {
    if (!i->SemaAnalyze(*this)) return nullptr;
  }
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(VarDefAST &ast) {
  // handle array type
  auto type = HandleArray(var_type_, ast.arr_lens(), ast.id(), false);
  if (!type) return nullptr;
  // check type of initializer
  if (ast.init()) {
    const auto &log = ast.init()->logger();
    auto init = ast.init()->SemaAnalyze(*this);
    if (!init) return nullptr;
    if (init->IsArray() && !init->GetLength()) {
      // initializer list
      if (!type->IsArray() ||
          !var_type_->CanAccept(init->GetDerefedType())) {
        return LogError(log, "invalid initializer list", ast.id());
      }
    }
    else if (!type->CanAccept(init)) {
      LogError(log, "type mismatch when initializing", ast.id());
    }
  }
  // check if is conflicted
  if (symbols_->GetItem(ast.id(), false)) {
    return LogError(ast.logger(), "symbol has already been defined",
                    ast.id());
  }
  // add to environment
  symbols_->AddItem(ast.id(), type);
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(InitListAST &ast) {
  // NOTE: initializer list can only be used to initialize arrays
  //       and will return a zero-length, 1 dimension array type
  TypePtr type;
  for (const auto &i : ast.exprs()) {
    // get expression type
    auto expr = i->SemaAnalyze(*this);
    if (!expr) return nullptr;
    // update final type
    if (!type) {
      type = expr;
    }
    else if (type->IsInteger() && expr->IsInteger()) {
      type = GetCommonType(type, expr);
    }
    else if (!type->IsIdentical(expr)) {
      return LogError(i->logger(),
                      "element type mismatch in initializer list");
    }
  }
  // make array type
  type = std::make_shared<ArrayType>(std::move(type), 0, true);
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(FuncDeclAST &ast) {
  // get return type
  auto ret = ast.type()->SemaAnalyze(*this);
  if (!ret) return nullptr;
  if (in_func_) cur_ret_ = ret;
  // get type of parameters
  TypePtrList params;
  for (const auto &i : ast.params()) {
    auto param = i->SemaAnalyze(*this);
    if (!param) return nullptr;
    params.push_back(std::move(param));
  }
  // make function type
  auto type = std::make_shared<FuncType>(std::move(params),
                                         std::move(ret), true);
  // get target environment
  const auto &sym = in_func_ ? symbols_->outer() : symbols_;
  // check if is conflicted
  if (sym->GetItem(ast.id(), false)) {
    return LogError(ast.logger(), "function has already been defined",
                    ast.id());
  }
  // add to environment
  sym->AddItem(ast.id(), type);
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(FuncDefAST &ast) {
  // make new environment
  /*
    NOTE: structure of function's environment
          outer           <- global env
          |
          +- args/block   <- current env
  */
  auto env = NewEnv();
  auto in_func = xstl::Guard([this] { in_func_ = false; });
  in_func_ = true;
  // register function & parameters
  auto func = ast.header()->SemaAnalyze(*this);
  if (!func) return nullptr;
  cur_ret_ = func->GetReturnType(*func->GetArgsType());
  // analyze body
  if (!ast.body()->SemaAnalyze(*this)) return nullptr;
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(FuncParamAST &ast) {
  // get type
  auto type = ast.type()->SemaAnalyze(*this);
  if (!type) return nullptr;
  // handle array type
  type = HandleArray(std::move(type), ast.arr_lens(), ast.id(), true);
  if (!type) return nullptr;
  // add to environment
  if (in_func_) {
    // check if is conflicted
    if (symbols_->GetItem(ast.id(), false)) {
      return LogError(ast.logger(), "argument has already been declared",
                      ast.id());
    }
    symbols_->AddItem(ast.id(), type);
  }
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(StructDefAST &ast) {
  // reset status
  last_struct_name_ = ast.id();
  struct_elems_.clear();
  struct_elem_names_.clear();
  // create an empty struct type
  auto type = std::make_shared<StructType>(struct_elems_, ast.id(), false);
  // check if is conflicted
  if (structs_->GetItem(ast.id(), false)) {
    return LogError(ast.logger(), "struct has already been defined",
                    ast.id());
  }
  // add to environment
  structs_->AddItem(ast.id(), type);
  // get type of elements
  for (const auto &i : ast.elems()) {
    if (!i->SemaAnalyze(*this)) return nullptr;
  }
  // update struct type
  // TODO: circular reference!
  type->set_elems(std::move(struct_elems_));
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(EnumDefAST &ast) {
  // analyze elements
  for (const auto &i : ast.elems()) {
    if (!i->SemaAnalyze(*this)) return nullptr;
  }
  // check if is conflicted
  if (enums_->GetItem(ast.id(), false)) {
    return LogError(ast.logger(), "enumeration has already been defined",
                    ast.id());
  }
  // add to environment
  enums_->AddItem(ast.id(), enum_base_->GetValueType(false));
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(TypeAliasAST &ast) {
  // get type
  auto type = ast.type()->SemaAnalyze(*this);
  if (!type) return nullptr;
  // check if is conflicted
  if (aliases_->GetItem(ast.id(), false)) {
    return LogError(ast.logger(), "user type has already been defined",
                    ast.id());
  }
  // add to environment
  enums_->AddItem(ast.id(), std::move(type));
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(StructElemAST &ast) {
  // get base type
  auto base = ast.type()->SemaAnalyze(*this);
  if (!base) return nullptr;
  // check if is recursive type
  if (base->IsStruct() && base->GetTypeId() == last_struct_name_) {
    return LogError(ast.logger(), "recursive type is not allowed");
  }
  struct_elem_base_ = base;
  // analyze definitions
  for (const auto &i : ast.defs()) {
    if (!i->SemaAnalyze(*this)) return nullptr;
  }
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(StructElemDefAST &ast) {
  // check if name conflicted
  if (!struct_elem_names_.insert(ast.id()).second) {
    return LogError(ast.logger(), "conflicted struct element name",
                    ast.id());
  }
  // handle array type
  auto type = HandleArray(struct_elem_base_, ast.arr_lens(),
                          ast.id(), false);
  if (!type) return nullptr;
  // add to elements
  struct_elems_.push_back({std::string(ast.id()), type});
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(EnumElemAST &ast) {
  // check initializer
  const auto &expr = ast.expr();
  if (expr) {
    auto init = expr->SemaAnalyze(*this);
    if (!init || !enum_base_->CanAccept(init)) {
      return LogError(expr->logger(), "invalid enumerator initializer");
    }
  }
  // check if is conflicted
  if (symbols_->GetItem(ast.id(), false)) {
    return LogError(ast.logger(), "enumerator has already been defined",
                    ast.id());
  }
  // add to environment
  symbols_->AddItem(ast.id(), enum_base_);
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(BlockAST &ast) {
  // make new environment when not in function
  auto guard = !in_func_ ? NewEnv() : xstl::Guard(nullptr);
  // ananlyze statements
  for (const auto &i : ast.stmts()) {
    if (!i->SemaAnalyze(*this)) return nullptr;
  }
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(IfElseAST &ast) {
  // analyze condition
  auto cond = ast.cond()->SemaAnalyze(*this);
  if (!cond || !(cond->IsInteger() || cond->IsPointer())) {
    return LogError(ast.cond()->logger(),
                    "condition must be an integer or a pointer");
  }
  // analyze branches
  if (!ast.then()->SemaAnalyze(*this)) return nullptr;
  if (ast.else_then() && !ast.else_then()->SemaAnalyze(*this)) {
    return nullptr;
  }
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(WhileAST &ast) {
  // analyze condition
  auto cond = ast.cond()->SemaAnalyze(*this);
  if (!cond || !(cond->IsInteger() || cond->IsPointer())) {
    return LogError(ast.cond()->logger(),
                    "condition must be an integer or a pointer");
  }
  // analyze body
  ++in_loop_;
  if (!ast.body()->SemaAnalyze(*this)) return nullptr;
  --in_loop_;
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(ControlAST &ast) {
  using Type = ControlAST::Type;
  switch (ast.type()) {
    case Type::Break: case Type::Continue: {
      // check if is in a loop
      if (!in_loop_) {
        return LogError(ast.logger(),
                        "using break/continue outside the loop");
      }
      break;
    }
    case Type::Return: {
      assert(cur_ret_->IsVoid() || !cur_ret_->IsRightValue());
      // check if is compatible
      auto type = ast.expr() ? ast.expr()->SemaAnalyze(*this) : MakeVoid();
      if (!type || !cur_ret_->CanAccept(type)) {
        return LogError(ast.expr()->logger(), "invalid return statement");
      }
      break;
    }
    default: assert(false);
  }
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(BinaryAST &ast) {
  using Op = BinaryAST::Operator;
  // get lhs & rhs
  auto lhs = ast.lhs()->SemaAnalyze(*this);
  auto rhs = ast.rhs()->SemaAnalyze(*this);
  if (!lhs || !rhs) return nullptr;
  // preprocess some types
  if (lhs->IsVoid() || rhs->IsVoid()) {
    return LogError(ast.logger(), "invalid operation between void types");
  }
  // handle by operator
  TypePtr type;
  switch (ast.op()) {
    case Op::Add: case Op::Sub:
    case Op::Less: case Op::LessEq: case Op::Great: case Op::GreatEq: {
      if (lhs->IsPointer() || rhs->IsPointer()) {
        // pointer operation
        if (lhs->IsPointer() && rhs->IsInteger()) {
          type = lhs;
        }
        else if (rhs->IsPointer() && lhs->IsInteger() &&
                 ast.op() != Op::Sub) {
          type = rhs;
        }
        else {
          return LogError(ast.logger(), "invalid pointer operation");
        }
        break;
      }
      // fall through
    }
    case Op::Mul: case Op::Div: case Op::Mod:
    case Op::And: case Op::Or: case Op::Xor:
    case Op::Shl: case Op::Shr: case Op::LAnd: case Op::LOr: {
      // int binary operation
      if (lhs->IsInteger() && rhs->IsInteger()) {
        type = GetCommonType(lhs, rhs);
      }
      break;
    }
    case Op::Equal: case Op::NotEqual: {
      // binary operation between all types except structures
      if (!lhs->IsStruct() && lhs->IsIdentical(rhs)) {
        if (lhs->IsArray()) {
          ast.logger().LogWarning(
              "array comparison always evaluates to a constant value");
        }
        type = MakePrimType(PrimType::Type::Int32, true);
      }
      break;
    }
    case Op::Assign: {
      // binary operation between all types
      if (lhs->CanAccept(rhs)) type = lhs;
      break;
    }
    case Op::AssAdd: case Op::AssSub: {
      // pointer operation
      if (lhs->IsPointer() && !lhs->IsRightValue() && !lhs->IsConst() &&
          rhs->IsInteger()) {
        type = lhs;
        break;
      }
      // fall through
    }
    case Op::AssMul: case Op::AssDiv: case Op::AssMod:
    case Op::AssAnd: case Op::AssOr: case Op::AssXor:
    case Op::AssShl: case Op::AssShr: {
      // int binary operation
      if (lhs->IsInteger() && lhs->CanAccept(rhs)) type = lhs;
      break;
    }
    default: assert(false);
  }
  // check return type
  if (!type) return LogError(ast.logger(), "invalid binary operation");
  if (!BinaryAST::IsOperatorAssign(ast.op()) && !type->IsRightValue()) {
    type = type->GetValueType(true);
  }
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(CastAST &ast) {
  auto expr = ast.expr()->SemaAnalyze(*this);
  auto type = ast.type()->SemaAnalyze(*this);
  if (!expr || !type) return nullptr;
  // check if cast is valid
  if (!expr->CanCastTo(type)) {
    return LogError(ast.logger(), "invalid type casting");
  }
  return ast.set_ast_type(type->GetValueType(true));
}

TypePtr Analyzer::AnalyzeOn(UnaryAST &ast) {
  using Op = UnaryAST::Operator;
  // get operand
  auto opr = ast.opr()->SemaAnalyze(*this);
  if (!opr || opr->IsVoid()) {
    return LogError(ast.opr()->logger(), "invalid operand");
  }
  // handle by operator
  TypePtr type;
  switch (ast.op()) {
    case Op::Pos: case Op::Neg: case Op::Not: case Op::LNot: {
      if (opr->IsInteger()) type = opr;
      break;
    }
    case Op::Deref: {
      if (opr->IsPointer() || opr->IsArray()) type = opr->GetDerefedType();
      break;
    }
    case Op::Addr: {
      if (!opr->IsRightValue()) type = MakePointer(opr);
      break;
    }
    case Op::SizeOf: {
      type = MakePrimType(PrimType::Type::UInt32, true);
      break;
    }
    default: assert(false);
  }
  // check return type
  if (!type) return LogError(ast.logger(), "invalid unary operator");
  if (ast.op() != Op::Deref && !type->IsRightValue()) {
    type = type->GetValueType(true);
  }
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(IndexAST &ast) {
  // get expression
  auto expr = ast.expr()->SemaAnalyze(*this);
  if (!expr || !(expr->IsPointer() || expr->IsArray())) {
    return LogError(ast.expr()->logger(),
                    "expression is not subscriptable");
  }
  // get type of index
  auto index = ast.index()->SemaAnalyze(*this);
  if (!index || !index->IsInteger()) {
    return LogError(ast.index()->logger(), "invalid index");
  }
  // get return type
  auto type = expr->GetDerefedType();
  if (expr->IsArray()) {
    if (auto val = ast.index()->Eval(eval_)) {
      // check if out of bounds
      if (*val >= expr->GetLength()) {
        ast.index()->logger().LogWarning("subscript out of bounds");
      }
    }
  }
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(FuncCallAST &ast) {
  // get expression
  auto expr = ast.expr()->SemaAnalyze(*this);
  if (!expr) return nullptr;
  if (!expr || !expr->IsFunction()) {
    return LogError(ast.expr()->logger(), "calling a non-function");
  }
  // get arguments
  TypePtrList args;
  for (const auto &i : ast.args()) {
    auto arg = i->SemaAnalyze(*this);
    if (!arg) return nullptr;
    args.push_back(std::move(arg));
  }
  // check return type
  auto ret = expr->GetReturnType(args);
  if (!ret) return LogError(ast.logger(), "invalid function call");
  return ast.set_ast_type(ret->GetValueType(true));
}

TypePtr Analyzer::AnalyzeOn(AccessAST &ast) {
  // get expression
  auto expr = ast.expr()->SemaAnalyze(*this);
  if (!expr) return nullptr;
  // get dereferenced type
  if (ast.is_arrow()) {
    // check if is pointer
    if (!expr->IsPointer()) {
      return LogError(ast.expr()->logger(), "expression is not a pointer");
    }
    expr = expr->GetDerefedType();
  }
  // check if is valid
  if (!expr->IsStruct()) {
    return LogError(ast.expr()->logger(), "structure type required");
  }
  auto type = expr->GetElem(ast.id());
  if (!type) return LogError(ast.logger(), "member not found", ast.id());
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(IntAST &ast) {
  // make right value 'int32' type
  return ast.set_ast_type(MakePrimType(PrimType::Type::Int32, true));
}

TypePtr Analyzer::AnalyzeOn(CharAST &ast) {
  // make right value 'int8' type
  return ast.set_ast_type(MakePrimType(PrimType::Type::Int8, true));
}

TypePtr Analyzer::AnalyzeOn(StringAST &ast) {
  // make right value 'const int8*' type
  auto type = MakePrimType(PrimType::Type::Int8, true);
  type = std::make_shared<ConstType>(std::move(type));
  return ast.set_ast_type(MakePointer(std::move(type)));
}

TypePtr Analyzer::AnalyzeOn(IdAST &ast) {
  // query from environment
  auto type = symbols_->GetItem(ast.id());
  if (!type) return LogError(ast.logger(), "undefined symbol", ast.id());
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(PrimTypeAST &ast) {
  auto type = MakePrimType(ast.type(), false);
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(UserTypeAST &ast) {
  // query from environment
  auto type = aliases_->GetItem(ast.id());
  if (!type) return LogError(ast.logger(), "type undefined", ast.id());
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(StructTypeAST &ast) {
  // query from environment
  auto type = structs_->GetItem(ast.id());
  if (!type) return LogError(ast.logger(), "type undefined", ast.id());
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(EnumTypeAST &ast) {
  // query from environment
  auto type = enums_->GetItem(ast.id());
  if (!type) return LogError(ast.logger(), "type undefined", ast.id());
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(ConstTypeAST &ast) {
  // get base type
  auto base = ast.base()->SemaAnalyze(*this);
  if (!base) return nullptr;
  // make const type
  auto type = std::make_shared<ConstType>(std::move(base));
  return ast.set_ast_type(std::move(type));
}

TypePtr Analyzer::AnalyzeOn(PointerTypeAST &ast) {
  // get base type
  auto type = ast.base()->SemaAnalyze(*this);
  if (!type) return nullptr;
  // make pointer type
  for (std::size_t i = 0; i < ast.depth(); ++i) {
    type = MakePointer(std::move(type), false);
  }
  return ast.set_ast_type(std::move(type));
}
