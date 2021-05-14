#include "front/parser.h"

#include <stack>
#include <cassert>

#include "front/token.h"

using namespace mimic::front;
using namespace mimic::define;

namespace {

// table of operator's precedence
// -1 if it's not a binary/assign operator
const int kOpPrecTable[] = {MIMIC_OPERATORS(MIMIC_EXPAND_THIRD)};

// return precedence of specific operator
inline int GetOpPrec(Operator op) {
  return kOpPrecTable[static_cast<int>(op)];
}

// check if keyword is beginning of basic types
inline bool IsBasicType(Keyword key) {
  return key == Keyword::Void || key == Keyword::Unsigned ||
         key == Keyword::Int || key == Keyword::Char ||
         key == Keyword::Const;
}

// convert lexer-generated operator to binary operator
inline BinaryAST::Operator GetBinaryOp(Operator op) {
  using Op = BinaryAST::Operator;
  switch (op) {
    case Operator::Add: return Op::Add;
    case Operator::Sub: return Op::Sub;
    case Operator::Mul: return Op::Mul;
    case Operator::Div: return Op::Div;
    case Operator::Mod: return Op::Mod;
    case Operator::Equal: return Op::Equal;
    case Operator::NotEqual: return Op::NotEqual;
    case Operator::Less: return Op::Less;
    case Operator::LessEqual: return Op::LessEq;
    case Operator::Great: return Op::Great;
    case Operator::GreatEqual: return Op::GreatEq;
    case Operator::LogicAnd: return Op::LAnd;
    case Operator::LogicOr: return Op::LOr;
    case Operator::And: return Op::And;
    case Operator::Or: return Op::Or;
    case Operator::Xor: return Op::Xor;
    case Operator::Shl: return Op::Shl;
    case Operator::Shr: return Op::Shr;
    case Operator::Assign: return Op::Assign;
    case Operator::AssAdd: return Op::AssAdd;
    case Operator::AssSub: return Op::AssSub;
    case Operator::AssMul: return Op::AssMul;
    case Operator::AssDiv: return Op::AssDiv;
    case Operator::AssMod: return Op::AssMod;
    case Operator::AssAnd: return Op::AssAnd;
    case Operator::AssOr: return Op::AssOr;
    case Operator::AssXor: return Op::AssXor;
    case Operator::AssShl: return Op::AssShl;
    case Operator::AssShr: return Op::AssShr;
    default: assert(false); return Op::Add;
  }
}

}  // namespace

ASTPtr Parser::LogError(std::string_view message) {
  logger().LogError(message);
  return nullptr;
}

ASTPtr Parser::ParseCompUnit() {
  auto ast = ParseDeclDef(true);
  if (!ast) return LogError("invalid declaration/definition");
  return ast;
}

ASTPtr Parser::ParseDeclDef(bool parse_func_def) {
  auto log = logger();
  if (cur_token_ == Token::Keyword) {
    if (lexer_.key_val() == Keyword::Struct) {
      // type or struct def
      NextToken();
      // get id
      if (!ExpectId()) return nullptr;
      auto id = lexer_.id_val();
      NextToken();
      // check if is struct def
      if (IsTokenChar('{')) {
        return ParseStructDef(id);
      }
      else {
        // parse variable/function declaration
        auto type = MakeAST<StructTypeAST>(log, id);
        return ParseVarFunc(std::move(type), parse_func_def);
      }
    }
    else if (lexer_.key_val() == Keyword::Enum) {
      // type or enum def
      NextToken();
      if (IsTokenChar('{')) {
        // enum def without id
        return ParseEnumDef("");
      }
      else {
        // get id
        if (!ExpectId()) return nullptr;
        auto id = lexer_.id_val();
        NextToken();
        // check if is enum def
        if (IsTokenChar('{')) {
          return ParseEnumDef(id);
        }
        else {
          // parse variable/function declaration
          auto type = MakeAST<EnumTypeAST>(log, id);
          return ParseVarFunc(std::move(type), parse_func_def);
        }
      }
    }
    else if (IsBasicType(lexer_.key_val())) {
      // get type
      auto type = ParseType();
      if (!type) return nullptr;
      // parse variable/function declaration
      return ParseVarFunc(std::move(type), parse_func_def);
    }
  }
  // just return nullptr but don't log error
  return nullptr;
}

ASTPtr Parser::ParseVarFunc(ASTPtr type, bool parse_func_def) {
  auto log = logger();
  // get id
  if (!ExpectId()) return nullptr;
  auto id = lexer_.id_val();
  NextToken();
  // check if is function header
  if (IsTokenChar('(')) {
    auto header = ParseFuncHeader(std::move(type), id);
    if (parse_func_def && IsTokenChar('{')) {
      // function definition
      auto body = ParseBlock();
      if (!body) return nullptr;
      return MakeAST<FuncDefAST>(log, std::move(header), std::move(body));
    }
    else {
      // function declaration
      if (!ExpectChar(';')) return nullptr;
      return header;
    }
  }
  else {
    // variable declaration
    return ParseVarDecl(std::move(type), id);
  }
}

ASTPtr Parser::ParseVarDecl(ASTPtr type, const std::string &id) {
  auto log = logger();
  // get definitions
  ASTPtrList defs;
  auto cur_id = id;
  for (;;) {
    auto def = ParseVarDef(cur_id);
    if (!def) return nullptr;
    defs.push_back(std::move(def));
    // eat ','
    if (!IsTokenChar(',')) break;
    NextToken();
    // get next id
    if (!ExpectId()) return nullptr;
    cur_id = lexer_.id_val();
    NextToken();
  }
  // check & eat ';'
  if (!ExpectChar(';')) return nullptr;
  return MakeAST<VarDeclAST>(log, std::move(type), std::move(defs));
}

ASTPtr Parser::ParseVarDef(const std::string &id) {
  auto log = logger();
  // parse array def
  ASTPtrList arr_lens;
  if (!GetArrLens(arr_lens)) return nullptr;
  // parse initializer
  ASTPtr init;
  if (IsTokenOperator(Operator::Assign)) {
    NextToken();
    // get initializer
    init = ParseInitVal();
    if (!init) return nullptr;
  }
  return MakeAST<VarDefAST>(log, id, std::move(arr_lens), std::move(init));
}

ASTPtr Parser::ParseInitVal() {
  auto log = logger();
  if (IsTokenChar('{')) {
    NextToken();
    // get initializer list
    ASTPtrList exprs;
    if (!IsTokenChar('}')) {
      for (;;) {
        auto init = ParseInitVal();
        if (!init) return nullptr;
        exprs.push_back(std::move(init));
        // eat ','
        if (!IsTokenChar(',')) break;
        NextToken();
      }
    }
    // check & eat '}'
    if (!ExpectChar('}')) return nullptr;
    return MakeAST<InitListAST>(log, std::move(exprs));
  }
  else {
    return ParseExpr();
  }
}

ASTPtr Parser::ParseFuncHeader(ASTPtr type, const std::string &id) {
  auto log = logger();
  // eat '('
  NextToken();
  // get params
  ASTPtrList params;
  if (!IsTokenChar(')')) {
    for (;;) {
      auto param = ParseFuncParam();
      if (!param) return nullptr;
      params.push_back(std::move(param));
      // eat ','
      if (!IsTokenChar(',')) break;
      NextToken();
    }
  }
  // check & eat ')'
  if (!ExpectChar(')')) return nullptr;
  return MakeAST<FuncDeclAST>(log, std::move(type), id, std::move(params));
}

ASTPtr Parser::ParseFuncParam() {
  auto log = logger();
  // get type
  auto type = ParseType();
  if (!type) return nullptr;
  // get id
  if (!ExpectId()) return nullptr;
  auto id = lexer_.id_val();
  NextToken();
  // get array def
  ASTPtrList arr_lens;
  if (IsTokenChar('[')) {
    NextToken();
    // handle null array
    if (IsTokenChar(']')) {
      NextToken();
      arr_lens.push_back(nullptr);
    }
    else {
      auto expr = ParseExpr();
      if (!expr) return nullptr;
      arr_lens.push_back(std::move(expr));
      // check & eat ']'
      if (!ExpectChar(']')) return nullptr;
    }
  }
  if (!GetArrLens(arr_lens)) return nullptr;
  // create AST
  return MakeAST<FuncParamAST>(log, std::move(type), id,
                               std::move(arr_lens));
}

ASTPtr Parser::ParseStructDef(const std::string &id) {
  auto log = logger();
  // eat '{'
  NextToken();
  // get elements
  ASTPtrList elems;
  while (!IsTokenChar('}')) {
    auto elem = ParseStructElem();
    if (!elem) return nullptr;
    elems.push_back(std::move(elem));
  }
  // eat '}'
  NextToken();
  // check & eat ';'
  if (!ExpectChar(';')) return nullptr;
  return MakeAST<StructDefAST>(log, id, std::move(elems));
}

ASTPtr Parser::ParseEnumDef(const std::string &id) {
  auto log = logger();
  // eat '{'
  NextToken();
  // get elements
  ASTPtrList elems;
  while (cur_token_ == Token::Id) {
    auto elem = ParseEnumElem();
    if (!elem) return nullptr;
    elems.push_back(std::move(elem));
    // eat ','
    if (!IsTokenChar(',')) break;
    NextToken();
  }
  // check & eat '}'
  if (!ExpectChar('}')) return nullptr;
  // check & eat ';'
  if (!ExpectChar(';')) return nullptr;
  return MakeAST<EnumDefAST>(log, id, std::move(elems));
}

ASTPtr Parser::ParseStructElem() {
  auto log = logger();
  // get type
  auto type = ParseType();
  if (!type) return nullptr;
  // get definitions
  ASTPtrList defs;
  for (;;) {
    auto def = ParseStructElemDef();
    if (!def) return nullptr;
    defs.push_back(std::move(def));
    // eat ','
    if (!IsTokenChar(',')) break;
    NextToken();
  }
  // check & eat ';'
  if (!ExpectChar(';')) return nullptr;
  return MakeAST<StructElemAST>(log, std::move(type), std::move(defs));
}

ASTPtr Parser::ParseStructElemDef() {
  auto log = logger();
  // get id
  if (!ExpectId()) return nullptr;
  auto id = lexer_.id_val();
  NextToken();
  // get array def
  ASTPtrList arr_lens;
  if (!GetArrLens(arr_lens)) return nullptr;
  return MakeAST<StructElemDefAST>(log, id, std::move(arr_lens));
}

ASTPtr Parser::ParseEnumElem() {
  auto log = logger();
  // get id
  if (!ExpectId()) return nullptr;
  auto id = lexer_.id_val();
  NextToken();
  // get initializer
  ASTPtr expr;
  if (IsTokenOperator(Operator::Assign)) {
    NextToken();
    expr = ParseExpr();
    if (!expr) return nullptr;
  }
  return MakeAST<EnumElemAST>(log, id, std::move(expr));
}

ASTPtr Parser::ParseBlock() {
  auto log = logger();
  // check & eat '{'
  if (!ExpectChar('{')) return nullptr;
  // get block items
  ASTPtrList items;
  while (!IsTokenChar('}')) {
    auto item = ParseBlockItem();
    if (!item) return nullptr;
    items.push_back(std::move(item));
  }
  // eat '}'
  NextToken();
  return MakeAST<BlockAST>(log, std::move(items));
}

ASTPtr Parser::ParseBlockItem() {
  // get declarations/definitions
  auto ast = ParseDeclDef(false);
  if (Logger::error_num()) return nullptr;
  // get statements
  return ast ? std::move(ast) : ParseStmt();
}

ASTPtr Parser::ParseStmt() {
  if (IsTokenChar('{')) {
    // block
    return ParseBlock();
  }
  else if (IsTokenChar(';')) {
    // empty statement
    auto ast = MakeAST<IntAST>(0);
    // eat ';'
    NextToken();
    return ast;
  }
  else if (cur_token_ == Token::Keyword) {
    switch (lexer_.key_val()) {
      // if-else
      case Keyword::If: return ParseIfElse();
      // while
      case Keyword::While: return ParseWhile();
      // control statements
      case Keyword::Break: case Keyword::Continue:
      case Keyword::Return: return ParseControl();
      // size-of (bare expression)
      case Keyword::SizeOf: return ParseBare();
      // other
      default: return LogError("invalid statement");
    }
  }
  else {
    // bare expression
    return ParseBare();
  }
}

ASTPtr Parser::ParseBare() {
  // get expression
  auto expr = ParseExpr();
  if (!expr) return nullptr;
  // check & eat ';'
  if (!ExpectChar(';')) return nullptr;
  return expr;
}

ASTPtr Parser::ParseIfElse() {
  auto log = logger();
  // eat 'if'
  NextToken();
  // check & eat '('
  if (!ExpectChar('(')) return nullptr;
  // get condition
  auto cond = ParseExpr();
  if (!cond) return nullptr;
  // check & eat ')'
  if (!ExpectChar(')')) return nullptr;
  // get 'then' block
  auto then = ParseStmt();
  if (!then) return nullptr;
  // get 'else-then' block
  ASTPtr else_then;
  if (IsTokenKeyword(Keyword::Else)) {
    NextToken();
    else_then = ParseStmt();
    if (!else_then) return nullptr;
  }
  return MakeAST<IfElseAST>(log, std::move(cond), std::move(then),
                            std::move(else_then));
}

ASTPtr Parser::ParseWhile() {
  auto log = logger();
  // eat 'while'
  NextToken();
  // check & eat '('
  if (!ExpectChar('(')) return nullptr;
  // get condition
  auto cond = ParseExpr();
  if (!cond) return nullptr;
  // check & eat ')'
  if (!ExpectChar(')')) return nullptr;
  // get body
  auto body = ParseStmt();
  if (!body) return nullptr;
  return MakeAST<WhileAST>(log, std::move(cond), std::move(body));
}

ASTPtr Parser::ParseControl() {
  using Type = ControlAST::Type;
  auto log = logger();
  // get type of AST
  Type type;
  switch (lexer_.key_val()) {
    case Keyword::Break: type = Type::Break; break;
    case Keyword::Continue: type = Type::Continue; break;
    case Keyword::Return: type = Type::Return; break;
    default: assert(false);
  }
  NextToken();
  // get expression (return value)
  ASTPtr expr;
  if (type == Type::Return && !IsTokenChar(';')) {
    expr = ParseExpr();
    if (!expr) return nullptr;
  }
  // check & eat ';'
  if (!ExpectChar(';')) return nullptr;
  return MakeAST<ControlAST>(log, type, std::move(expr));
}

ASTPtr Parser::ParseExpr() {
  auto log = logger();
  std::stack<ASTPtr> oprs;
  std::stack<Operator> ops;
  // get the first expression
  auto expr = ParseCast();
  if (!expr) return nullptr;
  oprs.push(std::move(expr));
  // convert to postfix expression
  while (cur_token_ == Token::Operator) {
    // get operator
    auto op = lexer_.op_val();
    if (GetOpPrec(op) < 0) break;
    NextToken();
    // handle operator
    while (!ops.empty() && GetOpPrec(ops.top()) >= GetOpPrec(op)) {
      // create a new binary AST
      auto cur_op = GetBinaryOp(ops.top());
      ops.pop();
      auto rhs = std::move(oprs.top());
      oprs.pop();
      auto lhs = std::move(oprs.top());
      oprs.pop();
      oprs.push(MakeAST<BinaryAST>(log, cur_op, std::move(lhs),
                                   std::move(rhs)));
    }
    ops.push(op);
    // get next expression
    expr = ParseCast();
    if (!expr) return nullptr;
    oprs.push(std::move(expr));
  }
  // clear stacks
  while (!ops.empty()) {
    // create a new binary AST
    auto cur_op = GetBinaryOp(ops.top());
    ops.pop();
    auto rhs = std::move(oprs.top());
    oprs.pop();
    auto lhs = std::move(oprs.top());
    oprs.pop();
    oprs.push(MakeAST<BinaryAST>(log, cur_op, std::move(lhs),
                                 std::move(rhs)));
  }
  return std::move(oprs.top());
}

ASTPtr Parser::ParseCast() {
  auto log = logger();
  // check if need to handle type casting
  if (IsTokenChar('(')) {
    // eat '('
    NextToken();
    if (cur_token_ == Token::Keyword) {
      // get type
      auto type = ParseType();
      if (!type) return nullptr;
      // check & eat ')'
      if (!ExpectChar(')')) return nullptr;
      // get expression
      auto expr = ParseCast();
      if (!expr) return nullptr;
      // make type casting
      return MakeAST<CastAST>(log, std::move(type), std::move(expr));
    }
    else {
      return ParseFactor(true);
    }
  }
  else {
    return ParseUnary();
  }
}

ASTPtr Parser::ParseUnary() {
  using Op = UnaryAST::Operator;
  auto log = logger();
  // check if need to get operator
  if (cur_token_ == Token::Operator) {
    // get & check unary operator
    Op op;
    switch (lexer_.op_val()) {
      case Operator::Add: op = Op::Pos; break;
      case Operator::Sub: op = Op::Neg; break;
      case Operator::Not: op = Op::Not; break;
      case Operator::LogicNot: op = Op::LNot; break;
      case Operator::Mul: op = Op::Deref; break;
      case Operator::And: op = Op::Addr; break;
      default: return LogError("invalid unary operator");
    }
    NextToken();
    // get factor
    auto expr = ParseUnary();
    if (!expr) return nullptr;
    return MakeAST<UnaryAST>(log, op, std::move(expr));
  }
  else if (IsTokenKeyword(Keyword::SizeOf)) {
    NextToken();
    // get type or factor
    ASTPtr expr;
    if (IsTokenChar('(')) {
      // eat '('
      NextToken();
      if (cur_token_ == Token::Keyword) {
        expr = ParseType();
        // check & eat ')'
        if (!ExpectChar(')')) return nullptr;
      }
      else {
        expr = ParseFactor(true);
      }
    }
    else {
      expr = ParseFactor(false);
    }
    if (!expr) return nullptr;
    return MakeAST<UnaryAST>(log, Op::SizeOf, std::move(expr));
  }
  else {
    return ParseFactor(false);
  }
}

ASTPtr Parser::ParseFactor(bool in_bracket) {
  ASTPtr expr;
  if (in_bracket || IsTokenChar('(')) {
    // bracket expression
    if (!in_bracket) NextToken();
    expr = ParseExpr();
    if (!ExpectChar(')')) return nullptr;
  }
  else {
    // other values
    expr = ParseValue();
  }
  // parse the rest part
  while (expr) {
    if (IsTokenChar('[')) {
      expr = ParseIndex(std::move(expr));
    }
    else if (IsTokenChar('(')) {
      expr = ParseFuncCall(std::move(expr));
    }
    else if (IsTokenOperator(Operator::Access) ||
             IsTokenOperator(Operator::Arrow)) {
      expr = ParseAccess(std::move(expr));
    }
    else {
      break;
    }
  }
  return expr;
}

ASTPtr Parser::ParseValue() {
  ASTPtr val;
  switch (cur_token_) {
    case Token::Int: val = MakeAST<IntAST>(lexer_.int_val()); break;
    case Token::Char: val = MakeAST<CharAST>(lexer_.char_val()); break;
    case Token::String: val = MakeAST<StringAST>(lexer_.str_val()); break;
    case Token::Id: val = MakeAST<IdAST>(lexer_.id_val()); break;
    default: return LogError("invalid value");
  }
  NextToken();
  return val;
}

ASTPtr Parser::ParseIndex(ASTPtr expr) {
  auto log = logger();
  // eat '['
  NextToken();
  // get index
  auto index = ParseExpr();
  if (!index || !ExpectChar(']')) return nullptr;
  return MakeAST<IndexAST>(log, std::move(expr), std::move(index));
}

ASTPtr Parser::ParseFuncCall(ASTPtr expr) {
  auto log = logger();
  // eat '('
  NextToken();
  // get expression list
  ASTPtrList args;
  if (!IsTokenChar(')')) {
    for (;;) {
      auto arg = ParseExpr();
      if (!arg) return nullptr;
      args.push_back(std::move(arg));
      // eat ','
      if (!IsTokenChar(',')) break;
      NextToken();
    }
  }
  // check & eat ')'
  if (!ExpectChar(')')) return nullptr;
  return MakeAST<FuncCallAST>(log, std::move(expr), std::move(args));
}

ASTPtr Parser::ParseAccess(ASTPtr expr) {
  auto log = logger();
  // get AST type
  bool is_arrow = lexer_.op_val() != Operator::Access;
  NextToken();
  // get id
  if (!ExpectId()) return nullptr;
  auto id = lexer_.id_val();
  NextToken();
  return MakeAST<AccessAST>(log, is_arrow, std::move(expr), id);
}

ASTPtr Parser::ParseType() {
  auto log = logger();
  ASTPtr base;
  // get base type
  if (cur_token_ == Token::Keyword) {
    switch (lexer_.key_val()) {
      case Keyword::Struct: base = ParseStructType(); break;
      case Keyword::Enum: base = ParseEnumType(); break;
      case Keyword::Const: base = ParseConst(); break;
      default: base = ParsePrimType(); break;
    }
  }
  if (!base) return LogError("invalid type");
  // handle pointer type
  std::size_t depth = 0;
  for (; IsTokenOperator(Operator::Mul); ++depth) NextToken();
  return depth ? MakeAST<PointerTypeAST>(log, std::move(base), depth)
               : std::move(base);
}

ASTPtr Parser::ParsePrimType() {
  using Type = PrimTypeAST::Type;
  auto log = logger();
  // get type
  Type type;
  switch (lexer_.key_val()) {
    case Keyword::Void: type = Type::Void; break;
    case Keyword::Int: type = Type::Int32; break;
    case Keyword::Char: type = Type::Int8; break;
    case Keyword::Unsigned: type = Type::UInt32; break;
    default: return LogError("invalid primitive type");
  }
  NextToken();
  // handle 'unsigned int'
  if (type == Type::UInt32 && IsTokenKeyword(Keyword::Int)) NextToken();
  return MakeAST<PrimTypeAST>(log, type);
}

ASTPtr Parser::ParseStructType() {
  auto log = logger();
  // eat 'struct'
  NextToken();
  // get id
  if (!ExpectId()) return nullptr;
  auto id = lexer_.id_val();
  NextToken();
  return MakeAST<StructTypeAST>(log, id);
}

ASTPtr Parser::ParseEnumType() {
  auto log = logger();
  // eat 'enum'
  NextToken();
  // get id
  if (!ExpectId()) return nullptr;
  auto id = lexer_.id_val();
  NextToken();
  return MakeAST<EnumTypeAST>(log, id);
}

ASTPtr Parser::ParseConst() {
  auto log = logger();
  // eat 'const'
  NextToken();
  // get type
  auto type = ParseType();
  if (!type) return nullptr;
  return MakeAST<ConstTypeAST>(log, std::move(type));
}

bool Parser::GetArrLens(ASTPtrList &arr_lens) {
  while (IsTokenChar('[')) {
    NextToken();
    // get expression
    auto expr = ParseExpr();
    if (!expr) return false;
    arr_lens.push_back(std::move(expr));
    // check & eat ']'
    if (!ExpectChar(']')) return false;
  }
  return true;
}

bool Parser::ExpectChar(char c) {
  if (!IsTokenChar(c)) {
    std::string msg = "expected '";
    msg = msg + c + "'";
    LogError(msg.c_str());
    return false;
  }
  NextToken();
  return true;
}

bool Parser::ExpectId() {
  if (cur_token_ != Token::Id) {
    LogError("expected identifier");
    return false;
  }
  return true;
}
