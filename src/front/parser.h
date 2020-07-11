#ifndef MIMIC_FRONT_PARSER_H_
#define MIMIC_FRONT_PARSER_H_

#include <string_view>
#include <string>

#include "front/lexer.h"
#include "define/ast.h"
#include "front/token.h"

namespace mimic::front {

class Parser {
 public:
  Parser(Lexer &lexer) : lexer_(lexer) { Reset(); }

  // reset parser status
  void Reset() {
    lexer_.Reset();
    ended_ = false;
    NextToken();
  }

  // get next AST from token stream
  define::ASTPtr ParseNext() {
    if (cur_token_ == Token::End) {
      ended_ = true;
      return nullptr;
    }
    else {
      return ParseCompUnit();
    }
  }

  // getters
  // returns true if parser met EOF
  bool ended() const { return ended_; }

 private:
  // get next token from lexer and skip all EOLs
  Token NextToken() { return cur_token_ = lexer_.NextToken(); }

  // check if current token is a character (token type 'Other')
  bool IsTokenChar(char c) const {
    using namespace define;
    return (cur_token_ == Token::Other && lexer_.other_val() == c) ||
           (cur_token_ == Token::Id && lexer_.id_val().size() == 1 &&
            lexer_.id_val()[0] == c);
  }

  // check if current token is a keyword
  bool IsTokenKeyword(Keyword key) const {
    using namespace define;
    return cur_token_ == Token::Keyword && lexer_.key_val() == key;
  }

  // check if current token is an operator
  bool IsTokenOperator(Operator op) const {
    using namespace define;
    return cur_token_ == Token::Operator && lexer_.op_val() == op;
  }

  // create a new AST
  template <typename T, typename... Args>
  define::ASTPtr MakeAST(Args &&... args) {
    auto ast = std::make_unique<T>(std::forward<Args>(args)...);
    ast->set_logger(logger());
    return ast;
  }

  // create a new AST with specific logger
  template <typename T, typename... Args>
  define::ASTPtr MakeAST(Logger &logger, Args &&... args) {
    auto ast = std::make_unique<T>(std::forward<Args>(args)...);
    ast->set_logger(logger);
    return ast;
  }

  // log error and return null pointer
  define::ASTPtr LogError(std::string_view message);

  define::ASTPtr ParseCompUnit();
  define::ASTPtr ParseDeclDef(bool parse_func_def);
  define::ASTPtr ParseVarFunc(define::ASTPtr type, bool parse_func_def);

  define::ASTPtr ParseVarDecl(define::ASTPtr type, const std::string &id);
  define::ASTPtr ParseVarDef(const std::string &id);
  define::ASTPtr ParseInitVal();

  define::ASTPtr ParseFuncHeader(define::ASTPtr type,
                                 const std::string &id);
  define::ASTPtr ParseFuncParam();

  define::ASTPtr ParseStructDef(const std::string &id);
  define::ASTPtr ParseEnumDef(const std::string &id);
  define::ASTPtr ParseStructElem();
  define::ASTPtr ParseStructElemDef();
  define::ASTPtr ParseEnumElem();

  define::ASTPtr ParseBlock();
  define::ASTPtr ParseBlockItem();

  define::ASTPtr ParseStmt();
  define::ASTPtr ParseBare();
  define::ASTPtr ParseIfElse();
  define::ASTPtr ParseWhile();
  define::ASTPtr ParseControl();

  define::ASTPtr ParseExpr();
  define::ASTPtr ParseCast();
  define::ASTPtr ParseUnary();
  define::ASTPtr ParseFactor(bool in_bracket);

  define::ASTPtr ParseValue();
  define::ASTPtr ParseIndex(define::ASTPtr expr);
  define::ASTPtr ParseFuncCall(define::ASTPtr expr);
  define::ASTPtr ParseAccess(define::ASTPtr expr);

  define::ASTPtr ParseType();
  define::ASTPtr ParsePrimType();
  define::ASTPtr ParseStructType();
  define::ASTPtr ParseEnumType();
  define::ASTPtr ParseConst();

  // parse list of array length
  bool GetArrLens(define::ASTPtrList &arr_lens);
  // make sure current token is specific character and goto next token
  bool ExpectChar(char c);
  // make sure current token is identifier
  bool ExpectId();

  // private getters
  // current logger
  const Logger &logger() const { return lexer_.logger(); }

  Lexer &lexer_;
  Token cur_token_;
  bool ended_;
};

}  // namespace mimic::front

#endif  // MIMIC_FRONT_PARSER_H_
