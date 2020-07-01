%language "c++"
%defines
%locations
%define api.parser.class {Parser}

%code requires{
  #include <iostream>
  #include <string>
  #include <variant>
  #include <cstdio>
  #include "parserctx.h"
  using namespace mimic::front;
  using namespace mimic::define;
}

%parse-param { ParserCtx& ctx}

%define api.value.automove
%define api.value.type variant

%token VOID INT CHAR CONST IF ELSE WHILE CONTINUE BREAK RETURN
  LE_OP GE_OP EQ_OP NE_OP AND_OP OR_OP
  IDENT INT_CONST CHAR_CONST STRING_CONST
			
%nterm <ASTPtr> comp_unit decl const_decl btype const_def 
  const_init_val var_decl var_def init_val func_def
  func_fparam block block_item stmt
  exp cond lval primary_exp number unary_exp 
  mul_exp add_exp rel_exp eq_exp land_exp lor_exp const_exp

%nterm <ASTPtrList> const_defs arr_const_exps var_defs const_init_vals
  init_vals func_fparams block_items arr_exps func_rprarms

%nterm <std::string> ident

%token END 0 "end of file"

%code provides{
  extern int yylex(
    yy::Parser::semantic_type* yylval,
    yy::location* yyloc);

  extern FILE* yyin;
  extern char *yytext;
}

%%

comp_unit
  : decl {ctx.ast=$1;YYACCEPT;}
  | func_def {ctx.ast=$1;YYACCEPT;}
  | END {ctx.ast=nullptr;}
  ;

decl
  : const_decl 
  | var_decl
  ;

btype
  : VOID {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<PrimTypeAST>(PrimTypeAST::Type::Void);}
  | INT {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<PrimTypeAST>(PrimTypeAST::Type::Int32);}
  ;

const_decl
  : CONST btype const_defs ';' {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<VarDeclAST>(ctx.MakeAST<ConstTypeAST>($2),$3);}
  ;

const_defs
  : const_def {$$.emplace_back($1);}
  | const_defs ',' const_def {$1.emplace_back($3);$$=$1;}
  ;

const_def
  : ident '=' const_init_val {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<VarDefAST>($1,ASTPtrList(),$3);}
  | ident arr_const_exps '=' const_init_val {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<VarDefAST>($1,$2,$4);}
  ;

const_init_val
  : const_exp
  | '{' '}' {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<InitListAST>(ASTPtrList());}
  | '{' const_init_vals '}' {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<InitListAST>($2);}
  ;

const_init_vals
  : const_init_val {$$.emplace_back($1);}
  | const_init_vals ',' const_init_val {$1.emplace_back($3);$$=$1;}
  ;

arr_const_exps
  : '[' const_exp ']' {$$.emplace_back($2);}
  | arr_const_exps '[' const_exp ']' {$1.emplace_back($3);$$=$1;}
  ;

var_decl
  : btype var_defs ';' {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<VarDeclAST>($1,$2);}
  ;

var_defs
  : var_def {$$.emplace_back($1);}
  | var_defs ',' var_def {$1.emplace_back($3);$$=$1;}
  ;

var_def 
  : ident {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<VarDefAST>($1,ASTPtrList(),nullptr);}
  | ident arr_const_exps {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<VarDefAST>($1,$2,nullptr);}
  | ident '=' init_val {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<VarDefAST>($1,ASTPtrList(),$3);}
  | ident arr_const_exps '=' init_val {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<VarDefAST>($1,$2,$4);}
  ;

init_val
  : exp 
  | '{' '}' {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<InitListAST>(ASTPtrList());}
  | '{' init_vals '}' {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<InitListAST>($2);}
  ;

init_vals
  : init_val {$$.emplace_back($1);}
  | init_vals ',' init_val {$1.emplace_back($3);$$=$1;}
  ;
  
func_def
  : btype ident '(' ')' block {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<FuncDefAST>(ctx.MakeAST<FuncDeclAST>($1,$2,ASTPtrList()),$5);}
  | btype ident '(' func_fparams ')' block {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<FuncDefAST>(ctx.MakeAST<FuncDeclAST>($1,$2,$4),$6);}
  ;


func_fparams
  : func_fparam {$$.emplace_back($1);}
  | func_fparams ',' func_fparam {$1.emplace_back($3);$$=$1;}
  ;

func_fparam
  : btype ident {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<FuncParamAST>($1,$2,ASTPtrList());} 
  | btype ident '[' ']' {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<FuncParamAST>($1,$2,ASTPtrList());} 
  | btype ident '[' ']' arr_exps {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<FuncParamAST>($1,$2,$5);} 

block
  : '{' '}' {$$=nullptr;}
  | '{' block_items '}' {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<BlockAST>($2);}
  ;

block_items
  : block_item {$$.emplace_back($1);}
  | block_items block_item {$1.emplace_back($2);$$=$1;}
  ;

block_item
  : decl 
  | stmt 
  ;

stmt 
  : lval '=' exp ';' {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Assign,$1,$3);}
  | ';' {$$=nullptr;}
  | exp ';'
  | block 
  | IF '(' cond ')' stmt {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<IfElseAST>($3,$5,nullptr);}
  | IF '(' cond ')' stmt ELSE stmt {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<IfElseAST>($3,$5,$7);}
  | WHILE '(' cond ')' stmt {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<WhileAST>($3,$5);}
  | BREAK ';'  {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<ControlAST>(ControlAST::Type::Break,nullptr);}
  | CONTINUE ';' {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<ControlAST>(ControlAST::Type::Continue,nullptr);}
  | RETURN ';' {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<ControlAST>(ControlAST::Type::Return,nullptr);}
  | RETURN exp ';'  {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<ControlAST>(ControlAST::Type::Return,$2);}
  ;

exp
  : add_exp
  ;

cond 
  : lor_exp
  ;

lval
  : ident {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<IdAST>($1);}
  | ident arr_exps {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<VarDefAST>($1,$2,nullptr);}
  ;

arr_exps
  : '[' exp ']' {$$.emplace_back($2);}
  | arr_exps '[' exp ']' {$1.emplace_back($3);$$=$1;}
  ;

primary_exp
  : '(' exp ')' {$$=$2;}
  | lval
  | number
  ;

number
  : INT_CONST {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<IntAST>(yyla.value.as<int>());}
  ;

unary_exp
  : primary_exp
  | ident '(' ')' {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<FuncCallAST>(ctx.MakeAST<IdAST>($1),ASTPtrList());}
  | ident '(' func_rprarms ')' {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<FuncCallAST>(ctx.MakeAST<IdAST>($1),$3);}
  | '+' unary_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<UnaryAST>(UnaryAST::Operator::Pos,$2);}
  | '-' unary_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<UnaryAST>(UnaryAST::Operator::Neg,$2);}
  | '!' unary_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<UnaryAST>(UnaryAST::Operator::Not,$2);}
  ;

func_rprarms
  : exp {$$.emplace_back($1);}
  | func_rprarms ',' exp {$1.emplace_back($3);$$=$1;}
  ;

mul_exp
  : unary_exp
  | mul_exp '*' unary_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Mul,$1,$3);}
  | mul_exp '/' unary_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Div,$1,$3);}
  | mul_exp '%' unary_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Mod,$1,$3);}
  ;

add_exp
  : mul_exp
  | add_exp '+' mul_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Add,$1,$3);}
  | add_exp '-' mul_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Sub,$1,$3);}
  ;

rel_exp
  : add_exp
  | rel_exp '<' add_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Less,$1,$3);}
  | rel_exp '>' add_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Great,$1,$3);}
  | rel_exp LE_OP add_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::LessEq,$1,$3);}
  | rel_exp GE_OP add_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::GreatEq,$1,$3);}
  ;

eq_exp
  : rel_exp
  | eq_exp EQ_OP rel_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Equal,$1,$3);}
  | eq_exp NE_OP rel_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::NotEqual,$1,$3);}
  ;

land_exp
  : eq_exp
  | land_exp AND_OP eq_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::LAnd,$1,$3);}
  ;

lor_exp
  : land_exp
  | lor_exp OR_OP land_exp {ctx.logger.set(@$.begin.line,@$.begin.column);$$=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::LOr,$1,$3);}
  ;

const_exp
  : add_exp
  ;

ident:
  IDENT {$$=yyla.value.as<std::string>();}
  ;

%%

/*
int main(int argc,char **argv){
  ParserCtx ctx(argv[1]);
  yyin = ctx.fp;
  yy::Parser parser(ctx);
#if YYDEBUG
  parser.set_debug_level(1);
#endif
  while(1){
    bool success=!parser.parse();
    std::cout<<std::boolalpha<<success<<std::endl;
    if(ctx.ast!=nullptr)
      ctx.ast->Dump(std::cout);
    else 
      break;
  }
  ctx.finish();
}
*/

namespace yy{

  void Parser::error(location const &loc,const std::string& s){
    std::cerr<<yytext<<std::endl;
    std::cerr<<"error at "<<loc.begin<<":"<<s<<std::endl;
  }
}