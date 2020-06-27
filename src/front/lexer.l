%option noyywrap

%{
#include <cstdio>
#include <cstdlib>
#include <string>
#include "parser.tab.hh"

void update_loc(yy::location* yyloc);
void comment();

#define YY_DECL int yylex( \
    yy::Parser::semantic_type* yylval, \
    yy::location* yyloc)

#define YY_USER_ACTION update_loc(yyloc);

typedef yy::Parser::token token;
%}

%option never-interactive

Ident [_a-zA-Z][_a-zA-Z0-9]*
Dec ([1-9][0-9]*)
Oct ([0][0-7]*)
Hex ([0][xX][0-9A-Fa-f]+)
CharConst (['](([\\]['])|([^']))*['])
StringConst (["](([\\]["])|([^"]))*["])
WhiteSpace [ \t\r\n]+

%%

"/*"			{ comment(); }
"//"[^\n]* { /* //-comment */ }

void			{ return(token::VOID); }
int			  { return(token::INT); }
char			{ return(token::CHAR); }
const			{ return(token::CONST); }
if		    { return(token::IF); }
else			{ return(token::ELSE); }
while			{ return(token::WHILE); }
continue	{ return(token::CONTINUE); }
break		  { return(token::BREAK); }
return		{ return(token::RETURN); }

"<="			{ return(token::LE_OP); }
">="			{ return(token::GE_OP); }
"=="			{ return(token::EQ_OP); }
"!="			{ return(token::NE_OP); }
"&&"			{ return(token::AND_OP); }
"||"			{ return(token::OR_OP); }
";"			  { return(';'); }
"{"		    { return('{'); }
"}"		    { return('}'); }
","			  { return(','); }
":"			  { return(':'); }
"="			  { return('='); }
"("			  { return('('); }
")"			  { return(')'); }
"["	      { return('['); }
"]"		    { return(']'); }
"!"				{ return('!'); }
"-"			  { return('-'); }
"+"			  { return('+'); }
"*"			  { return('*'); }
"/"			  { return('/'); }
"%"			  { return('%'); }
"<"			  { return('<'); }
">"			  { return('>'); }

{Ident}	            { yylval->build(std::string(yytext));return token::IDENT; }
({Hex}|{Oct}|{Dec})	{ yylval->build(atoi(yytext));return token::INT_CONST; }
{CharConst}         { yylval->build(yytext[1]);return token::CHAR_CONST; }
{StringConst}	      { yylval->build(std::string(yytext+1,yytext+yyleng-1));return token::STRING_CONST; }
{WhiteSpace}		    { }
%%

void update_loc(yy::location* yyloc){
  static int curr_line = 1;
  static int curr_col  = 1;

  yyloc->begin.line   = curr_line;
  yyloc->begin.column = curr_col;

  char * s; 
	for(s = yytext; *s != '\0'; s++){
    if(*s == '\n'){
      curr_line++;
      curr_col = 1;
    }
		else if (*s == '\t')
			curr_col += 4 - ((curr_col-1) % 4);
		else
      curr_col++;
  }
}

void comment(){
	char c, prev = 0;
	while ((c = yyinput())!=EOF){
		if (c == '/' && prev == '*')
			return;
		prev = c;
	}
}