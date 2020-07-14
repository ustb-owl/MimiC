#ifndef MIMIC_FRONT_LEXER_H_
#define MIMIC_FRONT_LEXER_H_

#include <istream>
#include <string_view>
#include <string>
#include <cstdint>

#include "front/token.h"
#include "front/logger.h"

namespace mimic::front {

class Lexer {
 public:
  Lexer() : logger_() { Reset(nullptr); }
  Lexer(std::istream *in) : logger_() { Reset(in); }

  // reset lexer status
  void Reset();
  // reset lexer status (including input stream)
  void Reset(std::istream *in);
  // get next token from input stream
  Token NextToken();

  // current logger
  const Logger &logger() const { return logger_; }
  // identifiers
  const std::string &id_val() const { return id_val_; }
  // integer values
  std::uint32_t int_val() const { return int_val_; }
  // string literals
  const std::string &str_val() const { return str_val_; }
  // character literals
  std::int8_t char_val() const { return char_val_; }
  // keywords
  Keyword key_val() const { return key_val_; }
  // operators
  Operator op_val() const { return op_val_; }
  // other characters
  char other_val() const { return other_val_; }

 private:
  bool IsEOF() { return !in_ || in_->eof(); }
  bool IsEOL() {
    return IsEOF() || last_char_ == '\n' || last_char_ == '\r';
  }
  void NextChar() {
    if (IsEOF()) return;
    *in_ >> last_char_;
    logger_.IncreaseColPos();
  }

  // print error message and return Token::Error
  Token LogError(std::string_view message);

  // read escape character from stream
  int ReadEscape();
  // skip spaces in stream
  void SkipSpaces();

  Token HandleId();
  Token HandleNum();
  Token HandleString();
  Token HandleChar();
  Token HandleOperator();
  Token HandleComment();
  Token HandleBlockComment();
  Token HandleEOL();

  std::istream *in_;
  Logger logger_;
  char last_char_;
  // value of token
  std::string id_val_, str_val_;
  std::uint32_t int_val_;
  std::int8_t char_val_;
  Keyword key_val_;
  Operator op_val_;
  char other_val_;
};

}  // namespace mimic::front

#endif  // MIMIC_FRONT_LEXER_H_
