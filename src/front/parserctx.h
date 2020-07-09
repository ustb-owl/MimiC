#ifndef YULANG_FRONT_PARSERCTX_H_
#define YULANG_FRONT_PARSERCTX_H_

#include "define/ast.h"
#include "front/logger.h"

namespace mimic::front {

struct ParserCtx {
  FILE* fp;
  Logger logger;
  define::ASTPtr ast;

  ParserCtx(char* file){
    fp=fopen(file,"r");
  };

  void finish(){
    fclose(fp);
    fp=NULL;
  }

  // create a new AST
  template <typename T, typename... Args>
  define::ASTPtr MakeAST(Args &&... args) {
    auto ast = std::make_unique<T>(std::forward<Args>(args)...);
    ast->set_logger(logger);
    return ast;
  }

  // log error and return null pointer
  define::ASTPtr LogError(std::string_view message){
      logger.LogError(message);
      return nullptr;
  };
};

}  // namespace yulang::front

#endif  // YULANG_FRONT_PARSERCTX_H_