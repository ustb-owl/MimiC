# MimiC

MimiC is a compiler of C subset (extended SysY language) by USTB NSCSCC team.

MimiC can compile extended SysY language to ARM assembly and C.

## Getting Started

MimiC requires:

* `cmake` 3.5 or later.
* C++ compiler supporting C++17 standard.

You can build MimiC from repository by executing the following commands:

```
$ git clone --recursive https://github.com/MaxXSoft/MimiC.git
$ cd MimiC && mkdir build && cd build
$ cmake .. && make -j8
```

## Features

* Hand written frontend.
* Strong typed IR in SSA form.
* Optimizer based on pass and pass manager.
* Auto-scheduling, multi-stage, iterative pass execution.
* Abstracted general purpose backend interface.
* Machine-level IR (MIR) for multi-architecture machine instruction abstraction.
* MIR based passes for multi-architecture assembly generation.

## Automated Testing Tools

See [MimiC-autotest](https://github.com/MaxXSoft/MimiC-autotest).

## EBNF of the Extended SysY Lang

```ebnf
comp_unit       ::= {decl | type_def | func_def};
decl            ::= var_decl | func_decl;
type_def        ::= struct_def | enum_def | type_alias;
func_def        ::= func_header block;

var_decl        ::= type var_def {"," var_def} ";";
var_def         ::= ID_VAL {"[" expr "]"} ["=" init_val];
init_val        ::= expr | "{" [init_val {"," init_val}] "}";

func_decl       ::= func_header ";";
func_header     ::= type ID_VAL "(" [func_params] ")";
func_params     ::= func_param {"," func_param};
func_param      ::= type ID_VAL ["[" [expr] "]" {"[" expr "]"}];

struct_def      ::= "struct" ID_VAL "{" {struct_elem} "}" ";";
enum_def        ::= "enum" [ID_VAL] "{" enum_elems "}" ";";
type_alias      ::= "typedef" type ID_VAL ";";
struct_elem     ::= type struct_elem_def {"," struct_elem_def} ";";
struct_elem_def ::= ID_VAL {"[" expr "]"};
enum_elems      ::= ID_VAL ["=" expr] ["," enum_elems] [","];

block           ::= "{" {block_item} "}";
block_item      ::= decl | type_def | stmt;

stmt            ::= bare | block | if_else | while | control;
bare            ::= expr ";";
if_else         ::= "if" "(" expr ")" stmt ["else" stmt];
while           ::= "while" "(" expr ")" stmt;
control         ::= ("break" | "continue" | ("return" [expr])) ";";

expr            ::= cast {bin_op cast};
cast            ::= {"(" type ")"} unary;
unary           ::= {unary_op} factor | "sizeof" (factor | "(" type ")");
factor          ::= value | index | func_call | access | "(" expr ")";

bin_op          ::= "+"   | "-"   | "*"   | "/"   | "%"   | "&"
                  | "|"   | "^"   | "&&"  | "||"  | "<<"  | ">>"
                  | "=="  | "!="  | "<"   | "<="  | ">"   | ">="
                  | "="   | "+="  | "-="  | "*="  | "/="  | "%="
                  | "&="  | "|="  | "^="  | "<<=" | ">>=";
unary_op        ::= "+"   | "-"   | "!"   | "~"   | "*"   | "&";

value           ::= INT_VAL | CHAR_VAL | STR_VAL | ID_VAL;
index           ::= expr "[" expr "]";
func_call       ::= expr "(" [expr {"," expr}] ")";
access          ::= factor ("." | "->") ID_VAL;

type            ::= prim_type | struct_type | enum_type | const | pointer
                  | user_type;
prim_type       ::= "void" | ["unsigned"] "int" | "char";
struct_type     ::= "struct" ID_VAL;
enum_type       ::= "enum" ID_VAL;
const           ::= "const" type;
pointer         ::= type "*" {"*"};
user_type       ::= ID_VAL;
```

## Copyright & License

Copyright (C) 2010-2020 MaxXSoft MaxXing, 2018-2020 USTB NSCSCC team.

License GPLv3.
