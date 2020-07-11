#include <iostream>

#include "front/logger.h"
#include "front/lexer.h"
#include "front/parser.h"

using namespace std;
using namespace mimic::front;

int main(int argc, const char *argv[]) {
  // get file name
  if (argc < 2) return 1;
  auto file = argv[1];
  Logger::set_file(file);
  // create lexer & parser
  Lexer lexer(file);
  Parser parser(lexer);
  // get ASTs
  while (auto ast = parser.ParseNext()) {
    ast->Dump(std::cout);
  }
  return Logger::error_num();
}
