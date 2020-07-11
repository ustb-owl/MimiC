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

namespace mimic::driver {

// top level class of compiler system
class Compiler {
 public:
  Compiler()
      : parser_(lexer_), ana_(eval_), dump_ast_(false), os_(&std::cout) {
    Reset();
  }

  // reset compiler
  void Reset();
  // open stream
  void Open(std::istream *in);
  // compile stream to IR, return false if failed
  bool CompileToIR();

  // setters
  void set_dump_ast(bool dump_ast) { dump_ast_ = dump_ast; }
  void set_ostream(std::ostream *os) {
    assert(os);
    os_ = os;
  }

 private:
  front::Lexer lexer_;
  front::Parser parser_;
  mid::Analyzer ana_;
  mid::Evaluator eval_;
  // options
  bool dump_ast_;
  std::ostream *os_;
};

}  // namespace mimic::driver

#endif  // MIMIC_DRIVER_COMPILER_H_
