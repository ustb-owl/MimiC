#include "mid/irbuilder.h"

#include <cassert>

#include "define/type.h"

using namespace mimic::mid;
using namespace mimic::define;

xstl::Guard IRBuilder::NewEnv() {
  vals_ = xstl::MakeNestedMap(vals_);
  return xstl::Guard([this] { vals_ = vals_->outer(); });
}

SSAPtr IRBuilder::CreateBinOp(BinaryAST::Operator op, const SSAPtr &lhs,
                              const SSAPtr &rhs) {
  using Op = BinaryAST::Operator;
  // handle assignment first
  if (BinaryAST::IsOperatorAssign(op)) {
    // get value
    auto val = rhs;
    // create assignment
    if (op != Op::Assign) {
      val = CreateBinOp(BinaryAST::GetDeAssignedOp(op), lhs, rhs);
    }
    module_.CreateStore(val, lhs);
    return lhs;
  }
  else {
    // perform implicit type casting
    auto lv = lhs, rv = rhs;
    const auto &lty = lhs->type(), &rty = rhs->type();
    if (lty->IsInteger() && rty->IsInteger()) {
      const auto &ty = GetCommonType(lty, rty);
      lv = module_.CreateCast(lv, ty);
      rv = module_.CreateCast(rv, ty);
    }
    // handle by operator
    switch (op) {
      case Op::Add: case Op::Sub: {
        if (lty->IsPointer() || rty->IsPointer()) {
          // generate index
          auto index = lty->IsPointer() ? rv : lv;
          if (op == Op::Sub) index = module_.CreateNeg(index);
          // generate pointer operation
          const auto &ptr = lty->IsPointer() ? lv : rv;
          return module_.CreatePtrAccess(ptr, index);
        }
        else {
          return op == Op::Add ? module_.CreateAdd(lv, rv)
                               : module_.CreateSub(lv, rv);
        }
      }
      case Op::Mul: return module_.CreateMul(lv, rv);
      case Op::Div: return module_.CreateDiv(lv, rv);
      case Op::Mod: return module_.CreateRem(lv, rv);
      case Op::And: return module_.CreateAnd(lv, rv);
      case Op::Or: return module_.CreateOr(lv, rv);
      case Op::Xor: return module_.CreateXor(lv, rv);
      case Op::Shl: return module_.CreateShl(lv, rv);
      case Op::Shr: return module_.CreateShr(lv, rv);
      case Op::Equal: return module_.CreateEqual(lv, rv);
      case Op::NotEqual: return module_.CreateNotEq(lv, rv);
      case Op::Less: return module_.CreateLess(lv, rv);
      case Op::LessEq: return module_.CreateLessEq(lv, rv);
      case Op::Great: return module_.CreateGreat(lv, rv);
      case Op::GreatEq: return module_.CreateGreatEq(lv, rv);
      default: assert(false); return nullptr;
    }
  }
}

void IRBuilder::Reset() {
  module_.Reset();
  vals_ = xstl::MakeNestedMap<std::string, SSAPtr>();
  in_func_ = false;
  funcs_.clear();
  assert(break_cont_.empty());
}

SSAPtr IRBuilder::GenerateOn(VarDeclAST &ast) {
  auto context = module_.SetContext(ast.logger());
  for (const auto &i : ast.defs()) i->GenerateIR(*this);
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(VarDefAST &ast) {
  auto context = module_.SetContext(ast.logger());
  const auto &type = ast.ast_type();
  const auto &init = ast.init();
  SSAPtr val;
  // check if is global definition
  if (vals_->is_root()) {
    // global variables/constants
    auto var = module_.CreateGlobalVar(LinkageTypes::External,
                                       !type->IsConst(), ast.id(), type);
    if (init) {
      if (init->IsLiteral()) {
        // generate initializer
        auto expr = module_.CreateCast(init->GenerateIR(*this), type);
        var->set_init(expr);
      }
      else {
        // generate initialization instructions
        auto ctor = module_.EnterGlobalCtor();
        module_.CreateStore(init->GenerateIR(*this), var);
      }
    }
    val = var;
  }
  else {
    // local variables/constants
    auto alloca = module_.CreateAlloca(type);
    if (init) module_.CreateStore(init->GenerateIR(*this), alloca);
    val = alloca;
  }
  // add to environment
  vals_->AddItem(ast.id(), val);
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(InitListAST &ast) {
  auto context = module_.SetContext(ast.logger());
  const auto &type = ast.ast_type();
  if (ast.IsLiteral()) {
    if (ast.exprs().empty()) {
      // generate constant zero
      return module_.GetZero(type);
    }
    else {
      // generate all elements
      SSAPtrList exprs;
      for (std::size_t i = 0; i < type->GetLength(); ++i) {
        auto elem_ty = type->GetElem(i);
        SSAPtr expr;
        if (i < ast.exprs().size()) {
          expr = ast.exprs()[i]->GenerateIR(*this);
          expr = module_.CreateCast(expr, elem_ty);
        }
        else {
          expr = module_.GetZero(elem_ty);
        }
        exprs.push_back(std::move(expr));
      }
      // generate constant array
      assert(type->IsArray());
      return module_.GetArray(exprs, type);
    }
  }
  else {
    // create a temporary alloca
    auto val = module_.CreateAlloca(type);
    // generate zero initializer
    auto zero = module_.GetZero(type);
    module_.CreateStore(zero, val);
    // generate elements
    for (std::size_t i = 0; i < ast.exprs().size(); ++i) {
      auto ty = type->GetElem(i);
      auto elem = ast.exprs()[i]->GenerateIR(*this);
      auto ptr = module_.CreateElemAccess(val, module_.GetInt32(i), ty);
      module_.CreateStore(elem, ptr);
    }
    // generate load
    return module_.CreateLoad(val);
  }
}

SSAPtr IRBuilder::GenerateOn(FuncDeclAST &ast) {
  auto context = module_.SetContext(ast.logger());
  // generate function declaration
  UserPtr func;
  auto it = funcs_.find(ast.id());
  if (it == funcs_.end()) {
    // create function declaration
    func = module_.CreateFunction(LinkageTypes::External, ast.id(),
                                  ast.ast_type());
    funcs_.insert({ast.id(), func});
    // add to environment
    const auto &vals = in_func_ ? vals_->outer() : vals_;
    vals->AddItem(ast.id(), func);
  }
  else {
    func = it->second;
  }
  // return if is just a declaration
  if (!in_func_) return nullptr;
  // generate arguments
  auto args_block = module_.CreateBlock(func, "args");
  module_.SetInsertPoint(args_block);
  std::size_t arg_index = 0;
  for (const auto &i : ast.params()) {
    auto arg = i->GenerateIR(*this);
    auto arg_ref = module_.CreateArgRef(func, arg_index++);
    module_.CreateStore(arg_ref, arg);
  }
  // generate return value
  const auto &type = ast.ast_type();
  auto ret_type = type->GetReturnType(*type->GetArgsType());
  ret_val_ = ret_type->IsVoid() ? nullptr : module_.CreateAlloca(ret_type);
  // generate exit block
  func_exit_ = module_.CreateBlock(func, "func_exit");
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(FuncDefAST &ast) {
  auto context = module_.SetContext(ast.logger());
  // make new environment
  auto env = NewEnv();
  in_func_ = true;
  // generate function definition
  ast.header()->GenerateIR(*this);
  // generate body
  ast.body()->GenerateIR(*this);
  // emit 'exit' block
  module_.CreateJump(func_exit_);
  module_.SetInsertPoint(func_exit_);
  if (ret_val_) {
    auto ret = module_.CreateLoad(ret_val_);
    module_.CreateReturn(ret);
  }
  else {
    module_.CreateReturn(nullptr);
  }
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(FuncParamAST &ast) {
  auto context = module_.SetContext(ast.logger());
  assert(in_func_);
  // create allocation for arguments
  auto alloca = module_.CreateAlloca(ast.ast_type());
  // add to envrionment
  vals_->AddItem(ast.id(), alloca);
  return alloca;
}

SSAPtr IRBuilder::GenerateOn(StructDefAST &ast) {
  // do nothing
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(EnumDefAST &ast) {
  // do nothing
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(TypeAliasAST &ast) {
  // do nothing
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(StructElemAST &ast) {
  // do nothing
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(StructElemDefAST &ast) {
  // do nothing
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(EnumElemAST &ast) {
  // do nothing
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(BlockAST &ast) {
  auto context = module_.SetContext(ast.logger());
  // make new environment when not in function
  auto guard = !in_func_ ? NewEnv() : xstl::Guard(nullptr);
  if (in_func_) in_func_ = false;
  // create new block
  const auto &cur_func = module_.GetInsertPoint()->parent();
  auto block = module_.CreateBlock(cur_func);
  module_.CreateJump(block);
  module_.SetInsertPoint(block);
  // generate statements
  for (const auto &i : ast.stmts()) i->GenerateIR(*this);
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(IfElseAST &ast) {
  auto context = module_.SetContext(ast.logger());
  // create basic blocks
  const auto &func = module_.GetInsertPoint()->parent();
  auto then_block = module_.CreateBlock(func, "if_then");
  auto else_block = module_.CreateBlock(func, "if_else");
  auto end_block = module_.CreateBlock(func, "if_end");
  // create conditional branch
  auto cond = ast.cond()->GenerateIR(*this);
  module_.CreateBranch(cond, then_block, else_block);
  // emit 'then' block
  module_.SetInsertPoint(then_block);
  ast.then()->GenerateIR(*this);
  module_.CreateJump(end_block);
  // emit 'else' block
  module_.SetInsertPoint(else_block);
  if (ast.else_then()) ast.else_then()->GenerateIR(*this);
  module_.CreateJump(end_block);
  // emit 'end' block
  module_.SetInsertPoint(end_block);
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(WhileAST &ast) {
  auto context = module_.SetContext(ast.logger());
  // create basic blocks
  const auto &func = module_.GetInsertPoint()->parent();
  auto cond_block = module_.CreateBlock(func, "while_cond");
  auto body_block = module_.CreateBlock(func, "while_body");
  auto end_block = module_.CreateBlock(func, "while_end");
  // add to break/continue stack
  break_cont_.push({end_block, cond_block});
  // create jump
  module_.CreateJump(cond_block);
  // emit 'cond' block
  module_.SetInsertPoint(cond_block);
  auto cond = ast.cond()->GenerateIR(*this);
  module_.CreateBranch(cond, body_block, end_block);
  // emit 'body' block
  module_.SetInsertPoint(body_block);
  ast.body()->GenerateIR(*this);
  module_.CreateJump(cond_block);
  // emit 'end' block
  module_.SetInsertPoint(end_block);
  // pop the top element of break/continue stack
  break_cont_.pop();
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(ControlAST &ast) {
  using Type = ControlAST::Type;
  auto context = module_.SetContext(ast.logger());
  // create basic block
  const auto &func = module_.GetInsertPoint()->parent();
  auto block = module_.CreateBlock(func);
  switch (ast.type()) {
    case Type::Break: case Type::Continue: {
      // generate target
      const auto &cur = break_cont_.top();
      auto target = ast.type() == Type::Break ? cur.first : cur.second;
      // generate jump
      module_.CreateJump(target);
      break;
    }
    case Type::Return: {
      // generate return value
      if (ast.expr()) {
        auto val = ast.expr()->GenerateIR(*this);
        module_.CreateStore(val, ret_val_);
      }
      // generate jump
      module_.CreateJump(func_exit_);
      break;
    }
    default: assert(false); break;
  }
  // emit new block
  module_.SetInsertPoint(block);
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(BinaryAST &ast) {
  using Op = BinaryAST::Operator;
  auto context = module_.SetContext(ast.logger());
  // generate lhs
  auto lhs = ast.lhs()->GenerateIR(*this);
  // check if is logic operator (perform short circuit)
  if (ast.op() == Op::LAnd || ast.op() == Op::LOr) {
    // create basic blocks
    const auto &func = module_.GetInsertPoint()->parent();
    auto rhs_block = module_.CreateBlock(func, "logic_rhs");
    auto end_block = module_.CreateBlock(func, "logic_end");
    // generate result value
    const auto &type = ast.ast_type();
    assert(type->IsInteger());
    auto result = module_.CreateAlloca(type);
    // handle by operator
    if (ast.op() == Op::LAnd) {
      module_.CreateStore(module_.GetInt(0, type), result);
      module_.CreateBranch(lhs, rhs_block, end_block);
    }
    else {  // LogicOr
      module_.CreateStore(module_.GetInt(1, type), result);
      module_.CreateBranch(lhs, end_block, rhs_block);
    }
    // emit 'rhs' block
    module_.SetInsertPoint(rhs_block);
    auto rhs = ast.rhs()->GenerateIR(*this);
    module_.CreateStore(rhs, result);
    module_.CreateJump(end_block);
    // emit 'end' block
    module_.SetInsertPoint(end_block);
    return module_.CreateLoad(result);
  }
  // normal binary operation
  auto rhs = ast.rhs()->GenerateIR(*this);
  return CreateBinOp(ast.op(), lhs, rhs);
}

SSAPtr IRBuilder::GenerateOn(CastAST &ast) {
  auto context = module_.SetContext(ast.logger());
  // generate expression
  auto expr = ast.expr()->GenerateIR(*this);
  // create type casting
  return module_.CreateCast(expr, ast.type()->ast_type());
}

SSAPtr IRBuilder::GenerateOn(UnaryAST &ast) {
  using Op = UnaryAST::Operator;
  auto context = module_.SetContext(ast.logger());
  // generate operand
  auto opr = ast.opr()->GenerateIR(*this);
  // normal unary operation
  switch (ast.op()) {
    case Op::Pos: return opr;
    case Op::Neg: return module_.CreateNeg(opr);
    case Op::LNot: return module_.CreateLogicNot(opr);
    case Op::Not: return module_.CreateNot(opr);
    case Op::Deref: return module_.CreateLoad(opr);
    case Op::Addr: return opr->GetAddr();
    default: assert(false); return nullptr;
  }
}

SSAPtr IRBuilder::GenerateOn(IndexAST &ast) {
  auto context = module_.SetContext(ast.logger());
  // generate expression & index
  auto expr = ast.expr()->GenerateIR(*this);
  auto index = ast.index()->GenerateIR(*this);
  // get type of expression
  auto expr_ty = ast.expr()->ast_type();
  auto elem_ty = expr_ty->GetDerefedType();
  // generate indexing operation
  SSAPtr ptr;
  if (expr_ty->IsArray()) {
    ptr = module_.CreateElemAccess(expr, index, expr_ty->GetDerefedType());
  }
  else {
    ptr = module_.CreatePtrAccess(expr, index);
  }
  // generate load
  return module_.CreateLoad(ptr);
}

SSAPtr IRBuilder::GenerateOn(FuncCallAST &ast) {
  auto context = module_.SetContext(ast.logger());
  // generate callee
  auto callee = ast.expr()->GenerateIR(*this);
  // generate arguments
  SSAPtrList args;
  for (const auto &i : ast.args()) {
    args.push_back(i->GenerateIR(*this));
  }
  // generate function call
  return module_.CreateCall(callee, args);
}

SSAPtr IRBuilder::GenerateOn(AccessAST &ast) {
  auto context = module_.SetContext(ast.logger());
  // generate expression
  auto expr = ast.expr()->GenerateIR(*this);
  auto expr_ty = ast.expr()->ast_type();
  if (ast.is_arrow()) {
    expr = module_.CreateLoad(expr);
    expr_ty = expr_ty->GetDerefedType();
  }
  assert(expr_ty->IsStruct());
  // get index of element
  auto index = expr_ty->GetElemIndex(ast.id());
  assert(index);
  // generate access operation
  auto elem_ty = expr_ty->GetElem(*index);
  auto index_val = module_.GetInt32(*index);
  auto ptr = module_.CreateElemAccess(expr, index_val, elem_ty);
  // generate load
  return module_.CreateLoad(ptr);
}

SSAPtr IRBuilder::GenerateOn(IntAST &ast) {
  auto context = module_.SetContext(ast.logger());
  return module_.GetInt(ast.value(), ast.ast_type());
}

SSAPtr IRBuilder::GenerateOn(CharAST &ast) {
  auto context = module_.SetContext(ast.logger());
  return module_.GetInt(ast.c(), ast.ast_type());
}

SSAPtr IRBuilder::GenerateOn(StringAST &ast) {
  auto context = module_.SetContext(ast.logger());
  return module_.GetString(ast.str(), ast.ast_type());
}

SSAPtr IRBuilder::GenerateOn(IdAST &ast) {
  auto context = module_.SetContext(ast.logger());
  // get value
  auto val = vals_->GetItem(ast.id());
  if (!val->type()->IsFunction()) val = module_.CreateLoad(val);
  return val;
}

SSAPtr IRBuilder::GenerateOn(PrimTypeAST &ast) {
  // do nothing
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(StructTypeAST &ast) {
  // do nothing
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(EnumTypeAST &ast) {
  // do nothing
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(ConstTypeAST &ast) {
  // do nothing
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(PointerTypeAST &ast) {
  // do nothing
  return nullptr;
}

SSAPtr IRBuilder::GenerateOn(UserTypeAST &ast) {
  // do nothing
  return nullptr;
}
