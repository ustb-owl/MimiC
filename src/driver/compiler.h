#ifndef MIMIC_DRIVER_COMPILER_H_
#define MIMIC_DRIVER_COMPILER_H_

#include <istream>
#include <ostream>
#include <iostream>
#include <cassert>

#include "front/lexer.h"
#include "front/parser.h"
#include "mid/analyzer.h"
#include "mid/eval.h"
#include "mid/irbuilder.h"
#include "opt/passman.h"
#include "back/codegen.h"

namespace mimic::driver {

// top level class of compiler system
class Compiler {
 public:
  Compiler()
      : parser_(lexer_), ana_(eval_),
        dump_ast_(false), dump_yuir_(false),
        dump_pass_info_(false), dump_code_(false),
        os_(&std::cout) {
    Reset();
  }

  // reset compiler
  void Reset();
  // open stream
  void Open(std::istream *in);
  // compile stream to IR, return false if failed
  void CompileToIR();
  // run passes on IRs
  void RunPasses();
  // generate target code
  void GenerateCode(back::CodeGen &gen);

  // setters
  void set_opt_level(std::size_t opt_level) {
    pass_man_.set_opt_level(opt_level);
  }
  void set_stage(opt::PassStage stage) { pass_man_.set_stage(stage); }
  void set_dump_ast(bool dump_ast) { dump_ast_ = dump_ast; }
  void set_dump_yuir(bool dump_yuir) { dump_yuir_ = dump_yuir; }
  void set_dump_pass_info(bool dump_pass_info) {
    dump_pass_info_ = dump_pass_info;
  }
  void set_dump_code(bool dump_code) { dump_code_ = dump_code; }
  void set_ostream(std::ostream *os) {
    assert(os);
    os_ = os;
  }

  // getters
  std::size_t opt_level() const { return pass_man_.opt_level(); }
  bool dump_ast() const { return dump_ast_; }
  bool dump_yuir() const { return dump_yuir_; }
  bool dump_pass_info() const { return dump_pass_info_; }
  bool dump_code() const { return dump_code_; }

 private:
  front::Lexer lexer_;
  front::Parser parser_;
  mid::Evaluator eval_;
  mid::Analyzer ana_;
  mid::IRBuilder irb_;
  opt::PassManager pass_man_;
  // options
  bool dump_ast_, dump_yuir_, dump_pass_info_, dump_code_;
  std::ostream *os_;
};

}  // namespace mimic::driver

#endif  // MIMIC_DRIVER_COMPILER_H_
