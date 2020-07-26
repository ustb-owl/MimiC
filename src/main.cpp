#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>

#include "version.h"

#include "front/logger.h"
#include "driver/compiler.h"
#include "opt/stage.h"
#include "back/asm/generator.h"
#include "back/c/generator.h"

#include "xstl/argparse.h"

using namespace std;
using namespace mimic::front;
using namespace mimic::driver;
using namespace mimic::opt;
using namespace mimic::back;

namespace {

xstl::ArgParser GetArgp() {
  xstl::ArgParser argp;
  argp.AddArgument<string>("input", "input source file");
  argp.AddOption<bool>("help", "h", "show this message", false);
  argp.AddOption<bool>("version", "v", "show version info", false);
  argp.AddOption<bool>("asm", "S", "dump ARM assembly",
                       false);
  argp.AddOption<bool>("opt-2", "O2", "enable level-2 optimization",
                       false);
  argp.AddOption<string>("output", "o", "output file, default to stdout",
                         "");
  argp.AddOption<bool>("verbose", "V", "use verbose output", false);
  argp.AddOption<bool>("warn-error", "Werror", "treat warnings as errors",
                       false);
  argp.AddOption<bool>("warn-all", "Wall", "enable all warnings",
                       false);
  argp.AddOption<bool>("dump-ast", "da", "dump AST to output", false);
  argp.AddOption<bool>("dump-ir", "di", "dump IR to output", false);
  argp.AddOption<string>("pass-stage", "ps",
                         "optimize until specific stage", "");
  return argp;
}

void PrintVersion() {
  cout << APP_NAME << " version " << APP_VERSION << endl;
  cout << "Compiler of the MimiC (extended SysY) programming language.";
  cout << endl << endl;
  cout << "Copyright (C) 2010-2020 MaxXing. License GPLv3.";
  cout << endl;
}

void ParseArgument(xstl::ArgParser &argp, int argc, const char *argv[]) {
  auto ret = argp.Parse(argc, argv);
  // check if need to exit program
  if (argp.GetValue<bool>("help")) {
    argp.PrintHelp();
    exit(0);
  }
  else if (argp.GetValue<bool>("version")) {
    PrintVersion();
    exit(0);
  }
  else if (!ret) {
    cerr << "invalid input, run '";
    cerr << argp.program_name() << " -h' for help" << endl;
    exit(1);
  }
}

// get pre-declared functions
istringstream GetPreDeclFuncs() {
  istringstream iss;
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
  // set up argument parser & parse argument
  auto argp = GetArgp();
  ParseArgument(argp, argc, argv);

  // initialize compiler
  Compiler comp;
  if (argp.GetValue<bool>("dump-ast")) {
    comp.set_dump_ast(true);
  }
  else if (argp.GetValue<bool>("dump-ir")) {
    comp.set_dump_yuir(true);
  }
  else {
    comp.set_dump_code(true);
  }
  comp.set_dump_pass_info(argp.GetValue<bool>("verbose"));
  comp.set_opt_level(argp.GetValue<bool>("opt-2") ? 2 : 0);

  // initialize pass stage
  auto stage_name = argp.GetValue<string>("pass-stage");
  if (!stage_name.empty()) {
    auto stage = GetStageByName(stage_name);
    if (stage == PassStage::None) {
      Logger::LogRawError("invalid stage name");
      return 1;
    }
    comp.set_stage(stage);
  }

  // initialize input stream & logger
  auto in_file = argp.GetValue<string>("input");
  ifstream ifs(in_file);
  if (!ifs.is_open()) {
    Logger::LogRawError("invalid input file");
    return 1;
  }
  Logger::set_file(in_file);
  Logger::ResetErrorNum(argp.GetValue<bool>("warn-all"),
                        argp.GetValue<bool>("warn-error"));

  // initialize output stream
  auto out_file = argp.GetValue<string>("output");
  ofstream ofs;
  if (!out_file.empty()) ofs.open(out_file);
  comp.set_ostream(out_file.empty() ? &cout : &ofs);

  // handle pre-declared functions
  auto iss = GetPreDeclFuncs();
  comp.Open(&iss);
  comp.CompileToIR();

  // compile input file
  comp.Open(&ifs);
  comp.CompileToIR();
  if (comp.dump_ast()) exit(0);
  comp.RunPasses();
  if (comp.dump_yuir()) exit(0);

  // generate code
  if (argp.GetValue<bool>("asm")) {
    // generate assembly
    asmgen::AsmCodeGen gen;
    auto succ = gen.SetTargetArch("aarch32");
    assert(succ);
    static_cast<void>(succ);
    gen.set_opt_level(comp.opt_level());
    comp.GenerateCode(gen);
  }
  else {
    // generate C code
    c::CCodeGen gen;
    comp.GenerateCode(gen);
  }
  return Logger::error_num();
}
