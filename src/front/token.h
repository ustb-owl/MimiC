#ifndef MIMIC_FRONT_TOKEN_H_
#define MIMIC_FRONT_TOKEN_H_

#include <cassert>

// all supported keywords
#define MIMIC_KEYWORDS(e) \
  e(Struct, "struct") e(Enum, "enum") e(TypeDef, "typedef") \
  e(If, "if") e(Else, "else") e(While, "while") e(SizeOf, "sizeof") \
  e(Break, "break") e(Continue, "continue") e(Return, "return") \
  e(Void, "void") e(Unsigned, "unsigned") \
  e(Int, "int") e(Char, "char") e(Const, "const")

// all supported operators
#define MIMIC_OPERATORS(e) \
  e(Add, "+", 90) e(Sub, "-", 90) e(Mul, "*", 100) e(Div, "/", 100) \
  e(Mod, "%", 100) e(Equal, "==", 60) e(NotEqual, "!=", 60) \
  e(Less, "<", 70) e(LessEqual, "<=", 70) e(Great, ">", 70) \
  e(GreatEqual, ">=", 70) e(LogicAnd, "&&", 20) e(LogicOr, "||", 10) \
  e(LogicNot, "!", -1) e(And, "&", 50) e(Or, "|", 30) e(Not, "~", -1) \
  e(Xor, "^", 40) e(Shl, "<<", 80) e(Shr, ">>", 80) \
  e(Access, ".", -1) e(Arrow, "->", -1) \
  e(Assign, "=", 0) e(AssAdd, "+=", 0) e(AssSub, "-=", 0) \
  e(AssMul, "*=", 0) e(AssDiv, "/=", 0) e(AssMod, "%=", 0) \
  e(AssAnd, "&=", 0) e(AssOr, "|=", 0) e(AssXor, "^=", 0) \
  e(AssShl, "<<=", 0) e(AssShr, ">>=", 0)

// expand first element to comma-separated list
#define MIMIC_EXPAND_FIRST(i, ...)       i,
// expand second element to comma-separated list
#define MIMIC_EXPAND_SECOND(i, j, ...)   j,
// expand third element to comma-separated list
#define MIMIC_EXPAND_THIRD(i, j, k, ...) k,

namespace mimic::front {

enum class Token {
  Error, End,
  Id, Int, String, Char,
  Keyword, Operator,
  Other,
};

enum class Keyword { MIMIC_KEYWORDS(MIMIC_EXPAND_FIRST) };
enum class Operator { MIMIC_OPERATORS(MIMIC_EXPAND_FIRST) };

}  // namespace mimic::front

#endif  // MIMIC_FRONT_TOKEN_H_
