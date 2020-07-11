#include "driver/compiler.h"

#include "front/logger.h"

using namespace mimic::driver;
using namespace mimic::front;

void Compiler::Reset() {
  // reset lexer & parser
  parser_.Reset();
  // reset the rest part
  ana_.Reset();
  eval_.Reset();
}

void Compiler::Open(std::istream *in) {
  // reset lexer & parser only
  lexer_.Reset(in);
  parser_.Reset();
}

bool Compiler::CompileToIR() {
  while (auto ast = parser_.ParseNext()) {
    // perform sematic analyze
    if (!ast->SemaAnalyze(ana_)) break;
    ast->Eval(eval_);
    if (dump_ast_) ast->Dump(*os_);
  }
  return !Logger::error_num();
}
