#include "driver/compiler.h"

#include <cstdlib>

#include "front/logger.h"

using namespace mimic::driver;
using namespace mimic::front;
using namespace mimic::back;

void Compiler::Reset() {
  // reset lexer & parser
  parser_.Reset();
  // reset the rest part
  ana_.Reset();
  eval_.Reset();
  irb_.Reset();
}

void Compiler::Open(std::istream *in) {
  // reset lexer & parser only
  lexer_.Reset(in);
  parser_.Reset();
}

void Compiler::CompileToIR() {
  while (auto ast = parser_.ParseNext()) {
    // perform sematic analyze
    if (!ast->SemaAnalyze(ana_)) break;
    ast->Eval(eval_);
    if (dump_ast_) ast->Dump(*os_);
    // generate IR
    ast->GenerateIR(irb_);
  }
  // check if need to exit
  auto err_num = Logger::error_num();
  if (err_num) std::exit(err_num);
}

void Compiler::RunPasses() {
  // run passes on IR
  if (dump_pass_info_) pass_man_.ShowInfo(std::cerr);
  irb_.module().RunPasses(pass_man_);
  // check if need to dump IR
  auto err_num = Logger::error_num();
  if (!err_num && dump_yuir_) irb_.module().Dump(*os_);
  if (err_num) std::exit(err_num);
}

void Compiler::GenerateCode(CodeGen &gen) {
  irb_.module().GenerateCode(gen);
  if (dump_code_) gen.Dump(*os_);
}
