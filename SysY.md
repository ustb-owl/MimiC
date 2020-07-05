# SysY 定义整理及扩展

本文档包括以下内容:

1. 大赛要求的 SysY 语言的基础定义;
2. 扩展的 SysY 语言定义;
3. 编译器的基本设计/结构;
4. 预期开发流程.

## SysY 语言基础定义

大赛的 EBNF 定义有一些瑕疵 (甚至错误), 修改后的 EBNF 定义如下:

```ebnf
comp_unit   ::= [comp_unit] (decl | func_def);
decl        ::= const_decl | var_decl;
func_def    ::= type ID_VAL "(" [func_params] ")" block;

const_decl  ::= "const" type const_def {"," const_def} ";";
var_decl    ::= type var_def {"," var_def} ";";
const_def   ::= ID_VAL {"[" expr "]"} "=" init_val;
var_def     ::= ID_VAL {"[" expr "]"} ["=" init_val];
init_val    ::= expr | "{" [init_val {"," init_val}] "}";

func_params ::= func_param {"," func_param};
func_param  ::= type ID_VAL ["[" "]" {"[" expr "]"}];
block       ::= "{" {block_item} "}";
block_item  ::= decl | stmt;

stmt        ::= bare | assign | block | if_else | while | control;
bare        ::= expr ";";
assign      ::= expr "=" expr ";";
if_else     ::= "if" "(" expr ")" stmt ["else" stmt];
while       ::= "while" "(" expr ")" stmt;
control     ::= ("break" | "continue" | ("return" [expr])) ";";

expr        ::= unary {bin_op unary};
unary       ::= [unary_op] factor;
factor      ::= value | index | func_call | "(" expr ")";

bin_op      ::= "+"   | "-"   | "*" | "/" | "%"
              | "=="  | "!="  | "<" | ">" | "<="  | ">=";
unary_op    ::= "+"   | "-";

value       ::= INT_VAL | ID_VAL;
index       ::= expr "[" expr "]";
func_call   ::= expr "(" [expr {"," expr}] ")";

type        ::= "void" | "int";
```

规定:

* 由大写字母组成的 EBNF 定义, 例如 `ID_VAL`, 由 Lexer 处理, 实际上是一个 Token;
* 由小写字母组成的 EBNF 定义, 例如 `const_decl`, 由 Parser 处理, 可以生成一棵 AST.

上述定义针对要求大赛要求做出了少许变更, 比如:

* 函数和变量/常量定义共用同一套类型 (EBNF 定义 `type`);
* 模糊表达式 (`expr`) 和常量表达式 (`const_expr`) 的定义;
* 模糊表达式 (`expr`) 和左值表达式 (`lval`) 的定义;
* 模糊表达式 (`expr`) 和条件表达式 (`cond`) 的定义.

这些变更可以减轻 Parser 实现的负担, 尽量将语义检查的任务分给语义分析部分, 同时让 Parser 更易维护.

## 扩展的 SysY 语言

原有的 SysY 语言本来就是参考 C99 标准, 精简之后定义出来的. 根据之前系统能力培养大赛硬件部分的比赛经验, 这次必然会有很多队伍实现功能上更复杂的 SysY 语言编译器. 我们来猜一猜清华大佬想干嘛, 目前有这些方向:

1. 更接近 C 语言的编译器实现, 甚至完整支持 C99;
2. 更优化的编译后端, 生成的代码有足够高的性能.

第一点带来的好处是, 这个编译器可能会具备编译其他开源 C 语言项目的能力. 比如 Fabrice Bellard 实现的 TCC 可被用来编译 Linux 内核, 虽然其生成代码的质量并不高, 但胜在完整. 这部分的实现主要是体力活.

第二点会在性能测试中占据不小的优势, 而且后端优化本身就是这个比赛的重点, 这部分的实现需要我们对编译优化和 ARM 架构有一定的理解; 当然如果把两点都做了的话, 那肯定是起飞了.

我个人认为, 本次比赛我们可以尝试实现编译器的自举. 也就是说, 我们完成的 SysY 编译器需要尽可能具备编译自己的能力:

* 首先, 我们必须对现有的 SysY 语言做出一定的扩展, 方便编译器支持一些便于我们实现一个编译器的特性. 这部分扩展不能随便做, 比如给 C 语言加个泛型, 那听起来就有点不太正常, 所以我们应该尽量往完整实现 C 标准的方向靠近.
* 其次, 除了用 C++ 实现的比赛用的 SysY 编译器, 我们还需要用 SysY 语言 (也就是裁剪后的 C 语言) 实现一个 SysY 语言的编译器, 这个编译器不需要有多高的性能, 也不需要生成多漂亮的代码, 只要功能正常就好了.

扩展后的 SysY 语言的 EBNF 定义如下:

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
block_item      ::= decl | stmt;

stmt            ::= bare | block | if_else | while | control;
bare            ::= expr ";";
if_else         ::= "if" "(" expr ")" stmt ["else" stmt];
while           ::= "while" "(" expr ")" stmt;
control         ::= ("break" | "continue" | ("return" [expr])) ";";

expr            ::= cast {bin_op cast};
cast            ::= {"(" type ")"} unary;
unary           ::= [unary_op] factor | "sizeof" (factor | "(" type ")");
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

相比扩展前, 扩展后的 SysY 语言新增了以下内容:

* 函数声明;
* 结构体 (`struct`) 和枚举 (`enum`);
* 类型别名 (`typedef`);
* `sizeof` 表达式;
* 强制/隐式类型转换;
* 更多的运算符;
* 更多的类型.

除此之外, 为了实现多文件的编译, 编译器需要支持 `#include` 等宏指令. 虽然严格意义上说, 宏指令的支持不应该有编译器去做, 而应该交给预处理器 (preprocessor). 编译器需要支持下列宏指令:

* `#include`: 编译器扫描到 include 时, 应当自动读取其指定的文件, 并且将这个文件的解析结果插入到当前位置. 参考 YuLang 的 `import` 语句实现;
* `#ifndef`, `#define`, `#endif`: 这部分是为了实现 include guards. 其中, `#define` 只需要支持定义符号即可, 不需要支持符号内容的定义以及宏替换;
* 其他 `#` 开头的指令: 遇到这些指令时, 直接忽略整行内容.

## 编译器设计

编译器在结构上大致可以分为四个部分:

1. **前端**: 由 Lexer 和 Parser 组成, 负责进行词法分析和语法分析;
2. **中端**: 由 Analyzer 和 IRBuilder 组成, 负责进行语义分析和 IR 生成;
3. **后端 (平台无关)**: 由 PassManager 等组成, 负责进行基于 IR 的平台无关优化;
4. **后端 (平台相关)**: 由 CodeGen 等组成, 负责进行平台相关优化和目标代码生成.

同时, 为了实现自举, 我们需要用扩展后的 SysY 语言实现一个 SysY 语言编译器. 该编译器基于语法指导分析, 且大致分为以下几个部分:

1. 词法分析和语法分析;
2. 类型检查;
3. 目标代码生成.

## 预期开发流程

分以下几个阶段:

### 第一阶段: 完成 SysY 语言基础编译器

根据编译器的结构划分, 我们同样可以将该阶段分为并行的四个部分:

1. **前端开发**: 实现 Lexer 和 Parser; Lexer 的输入为文件流, 输出为 Token 流; Parser 的输出为 AST (流);
2. **中端开发**: 实现 Analyzer 和 IRBuilder; Analyzer 的输入为 AST, 输出为标记过类型的 AST; IRBuilder 的输出为 IR, 为方便后端优化, 所有的 IR 需要被存储到某处, 记为 Module;
3. **IR 优化部分开发**: 基于 PassManager 实现多种不同的 Pass, 每个 Pass 的输入和输出均为 IR;
4. **目标代码生成部分开发**: 实现 CodeGen 和相关的优化设施, CodeGen 的输入为 IR, 输出为 ARM 汇编.

所以, 我们首先需要讨论并确定以下结构的定义:

* AST;
* 类型表示;
* IR.

然后基于上述定义, 各自完成四个部分的开发.

### 第二阶段: 完成扩展后的编译器

第二阶段需要在第一阶段全部完成后开始, 基于第一阶段的实现, 我们需要做如下扩展:

1. **前端开发**: 支持扩展后的新语法;
1. **中端开发**: 支持扩展后的新类型, 并且实现相关的类型检查, 生成对应 IR;
1. **IR 优化部分开发**: 如果添加了新的 IR, 需要在已经实现的 Pass 中考虑对新增 IR 的优化;
1. **目标代码生成部分开发**: 同上, 考虑对新增 IR 的优化和代码生成.

需要修改以下结构的定义:

* **AST**: 添加能够表示新语法的 AST;
* **类型表示**: 添加能够表示新语法中类型的类型表示;
* **IR**: 如果需要的话, 添加新的 IR.

由此可见, 我们在实现第一部分的过程中, 最好直接定义一种相对完备的 IR, 这样在第二阶段, 我们只需要关心对前端和中端的修改即可.

### 第三阶段: 实现自举

这部分可以放在最后展开, 也可以在一开始的时候就与主线开发并行展开. 基于扩展 SysY 语言开发的语法指导翻译编译器可参考 `c4`, `tcc` 等实现, 细节待定.
