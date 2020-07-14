#include <iostream>
#include <sstream>
#include <fstream>

#include "front/logger.h"
#include "driver/compiler.h"
#include "back/c/generator.h"

using namespace std;
using namespace mimic::front;
using namespace mimic::driver;
using namespace mimic::back::c;

namespace {

// get pre-declared functions
std::istringstream GetPreDeclFuncs() {
  std::istringstream iss;
  iss.str(
    "int getint();\n"
    "int getch();\n"
    "int getarray(int a[]);\n"
    "void putint(int a);\n"
    "void putch(int a);\n"
    "void putarray(int n, int a[]);\n"
    "void starttime();\n"
    "void stoptime();\n"
  );
  return iss;
}

}  // namespace

int main(int argc, const char *argv[]) {
  // open input file
  if (argc < 2) return 1;
  std::ifstream ifs(argv[1]);
  if (!ifs.is_open()) {
    Logger::LogRawError("invalid input file");
    return 1;
  }
  else {
    Logger::set_file(argv[1]);
  }
  // initialize compiler
  Compiler comp;
  comp.set_dump_code(true);
  comp.set_opt_level(3);
  // handle pre-declared functions
  auto iss = GetPreDeclFuncs();
  comp.Open(&iss);
  auto ret = comp.CompileToIR();
  assert(ret);
  // compile input file
  comp.Open(&ifs);
  comp.CompileToIR();
  comp.RunPasses();
  // generate C code
  CCodeGen gen;
  comp.GenerateCode(gen);
  return Logger::error_num();
}
