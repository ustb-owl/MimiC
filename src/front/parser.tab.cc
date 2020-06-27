// A Bison parser, made by GNU Bison 3.6.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2020 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.





#include "parser.tab.hh"




#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif


// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (false)
# endif


// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yy_stack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YYUSE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

namespace yy {
#line 136 "parser.tab.cc"

#if YYDEBUG || 0
  const char *
  Parser::symbol_name (symbol_kind_type yysymbol)
  {
    return yytname_[yysymbol];
  }
#endif // #if YYDEBUG || 0


  /// Build a parser object.
  Parser::Parser (ParserCtx& ctx_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      ctx (ctx_yyarg)
  {}

  Parser::~Parser ()
  {}

  Parser::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------------.
  | symbol kinds.  |
  `---------------*/

  // basic_symbol.
  template <typename Base>
  Parser::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value ()
    , location (that.location)
  {
    switch (this->kind ())
    {
      case 41: // comp_unit
      case 42: // decl
      case 43: // const_decl
      case 44: // btype
      case 45: // const_def
      case 46: // const_init_val
      case 47: // var_decl
      case 48: // var_def
      case 49: // init_val
      case 50: // func_def
      case 51: // func_fparam
      case 52: // block
      case 53: // block_item
      case 54: // stmt
      case 55: // exp
      case 56: // cond
      case 57: // lval
      case 58: // primary_exp
      case 59: // number
      case 60: // unary_exp
      case 61: // mul_exp
      case 62: // add_exp
      case 63: // rel_exp
      case 64: // eq_exp
      case 65: // land_exp
      case 66: // lor_exp
      case 67: // const_exp
        value.copy< ASTPtr > (YY_MOVE (that.value));
        break;

      case 68: // const_defs
      case 69: // arr_const_exps
      case 70: // var_defs
      case 71: // const_init_vals
      case 72: // init_vals
      case 73: // func_fparams
      case 74: // block_items
      case 75: // arr_exps
      case 76: // func_rprarms
        value.copy< ASTPtrList > (YY_MOVE (that.value));
        break;

      case 77: // ident
        value.copy< std::string > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

  }



  template <typename Base>
  Parser::symbol_kind_type
  Parser::basic_symbol<Base>::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }

  template <typename Base>
  bool
  Parser::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return this->kind () == symbol_kind::S_YYEMPTY;
  }

  template <typename Base>
  void
  Parser::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    switch (this->kind ())
    {
      case 41: // comp_unit
      case 42: // decl
      case 43: // const_decl
      case 44: // btype
      case 45: // const_def
      case 46: // const_init_val
      case 47: // var_decl
      case 48: // var_def
      case 49: // init_val
      case 50: // func_def
      case 51: // func_fparam
      case 52: // block
      case 53: // block_item
      case 54: // stmt
      case 55: // exp
      case 56: // cond
      case 57: // lval
      case 58: // primary_exp
      case 59: // number
      case 60: // unary_exp
      case 61: // mul_exp
      case 62: // add_exp
      case 63: // rel_exp
      case 64: // eq_exp
      case 65: // land_exp
      case 66: // lor_exp
      case 67: // const_exp
        value.move< ASTPtr > (YY_MOVE (s.value));
        break;

      case 68: // const_defs
      case 69: // arr_const_exps
      case 70: // var_defs
      case 71: // const_init_vals
      case 72: // init_vals
      case 73: // func_fparams
      case 74: // block_items
      case 75: // arr_exps
      case 76: // func_rprarms
        value.move< ASTPtrList > (YY_MOVE (s.value));
        break;

      case 77: // ident
        value.move< std::string > (YY_MOVE (s.value));
        break;

      default:
        break;
    }

    location = YY_MOVE (s.location);
  }

  // by_kind.
  Parser::by_kind::by_kind ()
    : kind_ (symbol_kind::S_YYEMPTY)
  {}

#if 201103L <= YY_CPLUSPLUS
  Parser::by_kind::by_kind (by_kind&& that)
    : kind_ (that.kind_)
  {
    that.clear ();
  }
#endif

  Parser::by_kind::by_kind (const by_kind& that)
    : kind_ (that.kind_)
  {}

  Parser::by_kind::by_kind (token_kind_type t)
    : kind_ (yytranslate_ (t))
  {}

  void
  Parser::by_kind::clear ()
  {
    kind_ = symbol_kind::S_YYEMPTY;
  }

  void
  Parser::by_kind::move (by_kind& that)
  {
    kind_ = that.kind_;
    that.clear ();
  }

  Parser::symbol_kind_type
  Parser::by_kind::kind () const YY_NOEXCEPT
  {
    return kind_;
  }

  Parser::symbol_kind_type
  Parser::by_kind::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }


  // by_state.
  Parser::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  Parser::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  Parser::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  Parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  Parser::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  Parser::symbol_kind_type
  Parser::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

  Parser::stack_symbol_type::stack_symbol_type ()
  {}

  Parser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case 41: // comp_unit
      case 42: // decl
      case 43: // const_decl
      case 44: // btype
      case 45: // const_def
      case 46: // const_init_val
      case 47: // var_decl
      case 48: // var_def
      case 49: // init_val
      case 50: // func_def
      case 51: // func_fparam
      case 52: // block
      case 53: // block_item
      case 54: // stmt
      case 55: // exp
      case 56: // cond
      case 57: // lval
      case 58: // primary_exp
      case 59: // number
      case 60: // unary_exp
      case 61: // mul_exp
      case 62: // add_exp
      case 63: // rel_exp
      case 64: // eq_exp
      case 65: // land_exp
      case 66: // lor_exp
      case 67: // const_exp
        value.YY_MOVE_OR_COPY< ASTPtr > (YY_MOVE (that.value));
        break;

      case 68: // const_defs
      case 69: // arr_const_exps
      case 70: // var_defs
      case 71: // const_init_vals
      case 72: // init_vals
      case 73: // func_fparams
      case 74: // block_items
      case 75: // arr_exps
      case 76: // func_rprarms
        value.YY_MOVE_OR_COPY< ASTPtrList > (YY_MOVE (that.value));
        break;

      case 77: // ident
        value.YY_MOVE_OR_COPY< std::string > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  Parser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case 41: // comp_unit
      case 42: // decl
      case 43: // const_decl
      case 44: // btype
      case 45: // const_def
      case 46: // const_init_val
      case 47: // var_decl
      case 48: // var_def
      case 49: // init_val
      case 50: // func_def
      case 51: // func_fparam
      case 52: // block
      case 53: // block_item
      case 54: // stmt
      case 55: // exp
      case 56: // cond
      case 57: // lval
      case 58: // primary_exp
      case 59: // number
      case 60: // unary_exp
      case 61: // mul_exp
      case 62: // add_exp
      case 63: // rel_exp
      case 64: // eq_exp
      case 65: // land_exp
      case 66: // lor_exp
      case 67: // const_exp
        value.move< ASTPtr > (YY_MOVE (that.value));
        break;

      case 68: // const_defs
      case 69: // arr_const_exps
      case 70: // var_defs
      case 71: // const_init_vals
      case 72: // init_vals
      case 73: // func_fparams
      case 74: // block_items
      case 75: // arr_exps
      case 76: // func_rprarms
        value.move< ASTPtrList > (YY_MOVE (that.value));
        break;

      case 77: // ident
        value.move< std::string > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
  Parser::stack_symbol_type&
  Parser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case 41: // comp_unit
      case 42: // decl
      case 43: // const_decl
      case 44: // btype
      case 45: // const_def
      case 46: // const_init_val
      case 47: // var_decl
      case 48: // var_def
      case 49: // init_val
      case 50: // func_def
      case 51: // func_fparam
      case 52: // block
      case 53: // block_item
      case 54: // stmt
      case 55: // exp
      case 56: // cond
      case 57: // lval
      case 58: // primary_exp
      case 59: // number
      case 60: // unary_exp
      case 61: // mul_exp
      case 62: // add_exp
      case 63: // rel_exp
      case 64: // eq_exp
      case 65: // land_exp
      case 66: // lor_exp
      case 67: // const_exp
        value.copy< ASTPtr > (that.value);
        break;

      case 68: // const_defs
      case 69: // arr_const_exps
      case 70: // var_defs
      case 71: // const_init_vals
      case 72: // init_vals
      case 73: // func_fparams
      case 74: // block_items
      case 75: // arr_exps
      case 76: // func_rprarms
        value.copy< ASTPtrList > (that.value);
        break;

      case 77: // ident
        value.copy< std::string > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    return *this;
  }

  Parser::stack_symbol_type&
  Parser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case 41: // comp_unit
      case 42: // decl
      case 43: // const_decl
      case 44: // btype
      case 45: // const_def
      case 46: // const_init_val
      case 47: // var_decl
      case 48: // var_def
      case 49: // init_val
      case 50: // func_def
      case 51: // func_fparam
      case 52: // block
      case 53: // block_item
      case 54: // stmt
      case 55: // exp
      case 56: // cond
      case 57: // lval
      case 58: // primary_exp
      case 59: // number
      case 60: // unary_exp
      case 61: // mul_exp
      case 62: // add_exp
      case 63: // rel_exp
      case 64: // eq_exp
      case 65: // land_exp
      case 66: // lor_exp
      case 67: // const_exp
        value.move< ASTPtr > (that.value);
        break;

      case 68: // const_defs
      case 69: // arr_const_exps
      case 70: // var_defs
      case 71: // const_init_vals
      case 72: // init_vals
      case 73: // func_fparams
      case 74: // block_items
      case 75: // arr_exps
      case 76: // func_rprarms
        value.move< ASTPtrList > (that.value);
        break;

      case 77: // ident
        value.move< std::string > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  Parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if YYDEBUG
  template <typename Base>
  void
  Parser::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YYUSE (yyoutput);
    if (yysym.empty ())
      yyo << "empty symbol";
    else
      {
        symbol_kind_type yykind = yysym.kind ();
        yyo << (yykind < YYNTOKENS ? "token" : "nterm")
            << ' ' << symbol_name (yykind) << " ("
            << yysym.location << ": ";
        YYUSE (yykind);
        yyo << ')';
      }
  }
#endif

  void
  Parser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  Parser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  Parser::yypop_ (int n)
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  Parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  Parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  Parser::debug_level_type
  Parser::debug_level () const
  {
    return yydebug_;
  }

  void
  Parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  Parser::state_type
  Parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
  Parser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  Parser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  Parser::operator() ()
  {
    return parse ();
  }

  int
  Parser::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << int (yystack_[0].state) << '\n';
    YY_STACK_PRINT ();

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[+yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token\n";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            yyla.kind_ = yytranslate_ (yylex (&yyla.value, &yyla.location));
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    if (yyla.kind () == symbol_kind::S_YYerror)
    {
      // The scanner already issued an error message, process directly
      // to error recovery.  But do not keep the error token as
      // lookahead, it is too special and may lead us to an endless
      // loop in error recovery. */
      yyla.kind_ = symbol_kind::S_YYUNDEF;
      goto yyerrlab1;
    }

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.kind ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.kind ())
      {
        goto yydefault;
      }

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", state_type (yyn), YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[+yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* Variants are always initialized to an empty instance of the
         correct type. The default '$$ = $1' action is NOT applied
         when using variants.  */
      switch (yyr1_[yyn])
    {
      case 41: // comp_unit
      case 42: // decl
      case 43: // const_decl
      case 44: // btype
      case 45: // const_def
      case 46: // const_init_val
      case 47: // var_decl
      case 48: // var_def
      case 49: // init_val
      case 50: // func_def
      case 51: // func_fparam
      case 52: // block
      case 53: // block_item
      case 54: // stmt
      case 55: // exp
      case 56: // cond
      case 57: // lval
      case 58: // primary_exp
      case 59: // number
      case 60: // unary_exp
      case 61: // mul_exp
      case 62: // add_exp
      case 63: // rel_exp
      case 64: // eq_exp
      case 65: // land_exp
      case 66: // lor_exp
      case 67: // const_exp
        yylhs.value.emplace< ASTPtr > ();
        break;

      case 68: // const_defs
      case 69: // arr_const_exps
      case 70: // var_defs
      case 71: // const_init_vals
      case 72: // init_vals
      case 73: // func_fparams
      case 74: // block_items
      case 75: // arr_exps
      case 76: // func_rprarms
        yylhs.value.emplace< ASTPtrList > ();
        break;

      case 77: // ident
        yylhs.value.emplace< std::string > ();
        break;

      default:
        break;
    }


      // Default location.
      {
        stack_type::slice range (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, range, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 2:
#line 50 "parser.y"
         {ctx.ast=YY_MOVE (yystack_[0].value.as < ASTPtr > ());YYACCEPT;}
#line 950 "parser.tab.cc"
    break;

  case 3:
#line 51 "parser.y"
             {ctx.ast=YY_MOVE (yystack_[0].value.as < ASTPtr > ());YYACCEPT;}
#line 956 "parser.tab.cc"
    break;

  case 4:
#line 52 "parser.y"
        {ctx.ast=nullptr;}
#line 962 "parser.tab.cc"
    break;

  case 5:
#line 56 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 968 "parser.tab.cc"
    break;

  case 6:
#line 57 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 974 "parser.tab.cc"
    break;

  case 7:
#line 61 "parser.y"
         {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<PrimTypeAST>(PrimTypeAST::Type::Void);}
#line 980 "parser.tab.cc"
    break;

  case 8:
#line 62 "parser.y"
        {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<PrimTypeAST>(PrimTypeAST::Type::Int32);}
#line 986 "parser.tab.cc"
    break;

  case 9:
#line 66 "parser.y"
                               {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<VarDeclAST>(ctx.MakeAST<ConstTypeAST>(YY_MOVE (yystack_[2].value.as < ASTPtr > ())),YY_MOVE (yystack_[1].value.as < ASTPtrList > ()));}
#line 992 "parser.tab.cc"
    break;

  case 10:
#line 70 "parser.y"
              {yylhs.value.as < ASTPtrList > ().emplace_back(YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 998 "parser.tab.cc"
    break;

  case 11:
#line 71 "parser.y"
                             {YY_MOVE (yystack_[2].value.as < ASTPtrList > ()).emplace_back(YY_MOVE (yystack_[0].value.as < ASTPtr > ()));yylhs.value.as < ASTPtrList > ()=YY_MOVE (yystack_[2].value.as < ASTPtrList > ());}
#line 1004 "parser.tab.cc"
    break;

  case 12:
#line 75 "parser.y"
                             {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<VarDefAST>(YY_MOVE (yystack_[2].value.as < std::string > ()),ASTPtrList(),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1010 "parser.tab.cc"
    break;

  case 13:
#line 76 "parser.y"
                                            {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<VarDefAST>(YY_MOVE (yystack_[3].value.as < std::string > ()),YY_MOVE (yystack_[2].value.as < ASTPtrList > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1016 "parser.tab.cc"
    break;

  case 14:
#line 80 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1022 "parser.tab.cc"
    break;

  case 15:
#line 81 "parser.y"
            {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<InitListAST>(ASTPtrList());}
#line 1028 "parser.tab.cc"
    break;

  case 16:
#line 82 "parser.y"
                            {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<InitListAST>(YY_MOVE (yystack_[1].value.as < ASTPtrList > ()));}
#line 1034 "parser.tab.cc"
    break;

  case 17:
#line 86 "parser.y"
                   {yylhs.value.as < ASTPtrList > ().emplace_back(YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1040 "parser.tab.cc"
    break;

  case 18:
#line 87 "parser.y"
                                       {YY_MOVE (yystack_[2].value.as < ASTPtrList > ()).emplace_back(YY_MOVE (yystack_[0].value.as < ASTPtr > ()));yylhs.value.as < ASTPtrList > ()=YY_MOVE (yystack_[2].value.as < ASTPtrList > ());}
#line 1046 "parser.tab.cc"
    break;

  case 19:
#line 91 "parser.y"
                      {yylhs.value.as < ASTPtrList > ().emplace_back(YY_MOVE (yystack_[1].value.as < ASTPtr > ()));}
#line 1052 "parser.tab.cc"
    break;

  case 20:
#line 92 "parser.y"
                                     {YY_MOVE (yystack_[3].value.as < ASTPtrList > ()).emplace_back(YY_MOVE (yystack_[1].value.as < ASTPtr > ()));yylhs.value.as < ASTPtrList > ()=YY_MOVE (yystack_[3].value.as < ASTPtrList > ());}
#line 1058 "parser.tab.cc"
    break;

  case 21:
#line 96 "parser.y"
                       {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<VarDeclAST>(YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[1].value.as < ASTPtrList > ()));}
#line 1064 "parser.tab.cc"
    break;

  case 22:
#line 100 "parser.y"
            {yylhs.value.as < ASTPtrList > ().emplace_back(YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1070 "parser.tab.cc"
    break;

  case 23:
#line 101 "parser.y"
                         {YY_MOVE (yystack_[2].value.as < ASTPtrList > ()).emplace_back(YY_MOVE (yystack_[0].value.as < ASTPtr > ()));yylhs.value.as < ASTPtrList > ()=YY_MOVE (yystack_[2].value.as < ASTPtrList > ());}
#line 1076 "parser.tab.cc"
    break;

  case 24:
#line 105 "parser.y"
          {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<VarDefAST>(YY_MOVE (yystack_[0].value.as < std::string > ()),ASTPtrList(),nullptr);}
#line 1082 "parser.tab.cc"
    break;

  case 25:
#line 106 "parser.y"
                         {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<VarDefAST>(YY_MOVE (yystack_[1].value.as < std::string > ()),YY_MOVE (yystack_[0].value.as < ASTPtrList > ()),nullptr);}
#line 1088 "parser.tab.cc"
    break;

  case 26:
#line 107 "parser.y"
                       {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<VarDefAST>(YY_MOVE (yystack_[2].value.as < std::string > ()),ASTPtrList(),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1094 "parser.tab.cc"
    break;

  case 27:
#line 108 "parser.y"
                                      {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<VarDefAST>(YY_MOVE (yystack_[3].value.as < std::string > ()),YY_MOVE (yystack_[2].value.as < ASTPtrList > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1100 "parser.tab.cc"
    break;

  case 28:
#line 112 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1106 "parser.tab.cc"
    break;

  case 29:
#line 113 "parser.y"
            {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<InitListAST>(ASTPtrList());}
#line 1112 "parser.tab.cc"
    break;

  case 30:
#line 114 "parser.y"
                      {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<InitListAST>(YY_MOVE (yystack_[1].value.as < ASTPtrList > ()));}
#line 1118 "parser.tab.cc"
    break;

  case 31:
#line 118 "parser.y"
             {yylhs.value.as < ASTPtrList > ().emplace_back(YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1124 "parser.tab.cc"
    break;

  case 32:
#line 119 "parser.y"
                           {YY_MOVE (yystack_[2].value.as < ASTPtrList > ()).emplace_back(YY_MOVE (yystack_[0].value.as < ASTPtr > ()));yylhs.value.as < ASTPtrList > ()=YY_MOVE (yystack_[2].value.as < ASTPtrList > ());}
#line 1130 "parser.tab.cc"
    break;

  case 33:
#line 123 "parser.y"
                              {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<FuncDefAST>(ctx.MakeAST<FuncDeclAST>(YY_MOVE (yystack_[4].value.as < ASTPtr > ()),YY_MOVE (yystack_[3].value.as < std::string > ()),ASTPtrList()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1136 "parser.tab.cc"
    break;

  case 34:
#line 124 "parser.y"
                                           {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<FuncDefAST>(ctx.MakeAST<FuncDeclAST>(YY_MOVE (yystack_[5].value.as < ASTPtr > ()),YY_MOVE (yystack_[4].value.as < std::string > ()),YY_MOVE (yystack_[2].value.as < ASTPtrList > ())),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1142 "parser.tab.cc"
    break;

  case 35:
#line 129 "parser.y"
                {yylhs.value.as < ASTPtrList > ().emplace_back(YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1148 "parser.tab.cc"
    break;

  case 36:
#line 130 "parser.y"
                                 {YY_MOVE (yystack_[2].value.as < ASTPtrList > ()).emplace_back(YY_MOVE (yystack_[0].value.as < ASTPtr > ()));yylhs.value.as < ASTPtrList > ()=YY_MOVE (yystack_[2].value.as < ASTPtrList > ());}
#line 1154 "parser.tab.cc"
    break;

  case 37:
#line 134 "parser.y"
                {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<FuncParamAST>(YY_MOVE (yystack_[1].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < std::string > ()),ASTPtrList());}
#line 1160 "parser.tab.cc"
    break;

  case 38:
#line 135 "parser.y"
                        {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<FuncParamAST>(YY_MOVE (yystack_[3].value.as < ASTPtr > ()),YY_MOVE (yystack_[2].value.as < std::string > ()),ASTPtrList());}
#line 1166 "parser.tab.cc"
    break;

  case 39:
#line 136 "parser.y"
                                 {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<FuncParamAST>(YY_MOVE (yystack_[4].value.as < ASTPtr > ()),YY_MOVE (yystack_[3].value.as < std::string > ()),YY_MOVE (yystack_[0].value.as < ASTPtrList > ()));}
#line 1172 "parser.tab.cc"
    break;

  case 40:
#line 139 "parser.y"
            {yylhs.value.as < ASTPtr > ()=nullptr;}
#line 1178 "parser.tab.cc"
    break;

  case 41:
#line 140 "parser.y"
                        {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<BlockAST>(YY_MOVE (yystack_[1].value.as < ASTPtrList > ()));}
#line 1184 "parser.tab.cc"
    break;

  case 42:
#line 144 "parser.y"
               {yylhs.value.as < ASTPtrList > ().emplace_back(YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1190 "parser.tab.cc"
    break;

  case 43:
#line 145 "parser.y"
                           {YY_MOVE (yystack_[1].value.as < ASTPtrList > ()).emplace_back(YY_MOVE (yystack_[0].value.as < ASTPtr > ()));yylhs.value.as < ASTPtrList > ()=YY_MOVE (yystack_[1].value.as < ASTPtrList > ());}
#line 1196 "parser.tab.cc"
    break;

  case 44:
#line 149 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1202 "parser.tab.cc"
    break;

  case 45:
#line 150 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1208 "parser.tab.cc"
    break;

  case 46:
#line 154 "parser.y"
                     {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Assign,YY_MOVE (yystack_[3].value.as < ASTPtr > ()),YY_MOVE (yystack_[1].value.as < ASTPtr > ()));}
#line 1214 "parser.tab.cc"
    break;

  case 47:
#line 155 "parser.y"
        {yylhs.value.as < ASTPtr > ()=nullptr;}
#line 1220 "parser.tab.cc"
    break;

  case 48:
#line 156 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[1].value.as < ASTPtr > ()); }
#line 1226 "parser.tab.cc"
    break;

  case 49:
#line 157 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1232 "parser.tab.cc"
    break;

  case 50:
#line 158 "parser.y"
                         {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<IfElseAST>(YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()),nullptr);}
#line 1238 "parser.tab.cc"
    break;

  case 51:
#line 159 "parser.y"
                                   {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<IfElseAST>(YY_MOVE (yystack_[4].value.as < ASTPtr > ()),YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1244 "parser.tab.cc"
    break;

  case 52:
#line 160 "parser.y"
                            {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<WhileAST>(YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1250 "parser.tab.cc"
    break;

  case 53:
#line 161 "parser.y"
               {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<ControlAST>(ControlAST::Type::Break,nullptr);}
#line 1256 "parser.tab.cc"
    break;

  case 54:
#line 162 "parser.y"
                 {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<ControlAST>(ControlAST::Type::Continue,nullptr);}
#line 1262 "parser.tab.cc"
    break;

  case 55:
#line 163 "parser.y"
               {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<ControlAST>(ControlAST::Type::Return,nullptr);}
#line 1268 "parser.tab.cc"
    break;

  case 56:
#line 164 "parser.y"
                    {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<ControlAST>(ControlAST::Type::Return,YY_MOVE (yystack_[1].value.as < ASTPtr > ()));}
#line 1274 "parser.tab.cc"
    break;

  case 57:
#line 168 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1280 "parser.tab.cc"
    break;

  case 58:
#line 172 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1286 "parser.tab.cc"
    break;

  case 59:
#line 176 "parser.y"
          {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<IdAST>(YY_MOVE (yystack_[0].value.as < std::string > ()));}
#line 1292 "parser.tab.cc"
    break;

  case 60:
#line 177 "parser.y"
                   {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<VarDefAST>(YY_MOVE (yystack_[1].value.as < std::string > ()),YY_MOVE (yystack_[0].value.as < ASTPtrList > ()),nullptr);}
#line 1298 "parser.tab.cc"
    break;

  case 61:
#line 181 "parser.y"
                {yylhs.value.as < ASTPtrList > ().emplace_back(YY_MOVE (yystack_[1].value.as < ASTPtr > ()));}
#line 1304 "parser.tab.cc"
    break;

  case 62:
#line 182 "parser.y"
                         {YY_MOVE (yystack_[3].value.as < ASTPtrList > ()).emplace_back(YY_MOVE (yystack_[1].value.as < ASTPtr > ()));yylhs.value.as < ASTPtrList > ()=YY_MOVE (yystack_[3].value.as < ASTPtrList > ());}
#line 1310 "parser.tab.cc"
    break;

  case 63:
#line 186 "parser.y"
                {yylhs.value.as < ASTPtr > ()=YY_MOVE (yystack_[1].value.as < ASTPtr > ());}
#line 1316 "parser.tab.cc"
    break;

  case 64:
#line 187 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1322 "parser.tab.cc"
    break;

  case 65:
#line 188 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1328 "parser.tab.cc"
    break;

  case 66:
#line 192 "parser.y"
              {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<IntAST>(yyla.value.as<int>());}
#line 1334 "parser.tab.cc"
    break;

  case 67:
#line 196 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1340 "parser.tab.cc"
    break;

  case 68:
#line 197 "parser.y"
                  {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<FuncCallAST>(ctx.MakeAST<IdAST>(YY_MOVE (yystack_[2].value.as < std::string > ())),ASTPtrList());}
#line 1346 "parser.tab.cc"
    break;

  case 69:
#line 198 "parser.y"
                               {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<FuncCallAST>(ctx.MakeAST<IdAST>(YY_MOVE (yystack_[3].value.as < std::string > ())),YY_MOVE (yystack_[1].value.as < ASTPtrList > ()));}
#line 1352 "parser.tab.cc"
    break;

  case 70:
#line 199 "parser.y"
                  {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<UnaryAST>(UnaryAST::Operator::Pos,YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1358 "parser.tab.cc"
    break;

  case 71:
#line 200 "parser.y"
                  {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<UnaryAST>(UnaryAST::Operator::Neg,YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1364 "parser.tab.cc"
    break;

  case 72:
#line 201 "parser.y"
                  {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<UnaryAST>(UnaryAST::Operator::Not,YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1370 "parser.tab.cc"
    break;

  case 73:
#line 205 "parser.y"
        {yylhs.value.as < ASTPtrList > ().emplace_back(YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1376 "parser.tab.cc"
    break;

  case 74:
#line 206 "parser.y"
                         {YY_MOVE (yystack_[2].value.as < ASTPtrList > ()).emplace_back(YY_MOVE (yystack_[0].value.as < ASTPtr > ()));yylhs.value.as < ASTPtrList > ()=YY_MOVE (yystack_[2].value.as < ASTPtrList > ());}
#line 1382 "parser.tab.cc"
    break;

  case 75:
#line 210 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1388 "parser.tab.cc"
    break;

  case 76:
#line 211 "parser.y"
                          {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Mul,YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1394 "parser.tab.cc"
    break;

  case 77:
#line 212 "parser.y"
                          {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Div,YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1400 "parser.tab.cc"
    break;

  case 78:
#line 213 "parser.y"
                          {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Mod,YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1406 "parser.tab.cc"
    break;

  case 79:
#line 217 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1412 "parser.tab.cc"
    break;

  case 80:
#line 218 "parser.y"
                        {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Add,YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1418 "parser.tab.cc"
    break;

  case 81:
#line 219 "parser.y"
                        {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Sub,YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1424 "parser.tab.cc"
    break;

  case 82:
#line 223 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1430 "parser.tab.cc"
    break;

  case 83:
#line 224 "parser.y"
                        {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Less,YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1436 "parser.tab.cc"
    break;

  case 84:
#line 225 "parser.y"
                        {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Great,YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1442 "parser.tab.cc"
    break;

  case 85:
#line 226 "parser.y"
                          {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::LessEq,YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1448 "parser.tab.cc"
    break;

  case 86:
#line 227 "parser.y"
                          {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::GreatEq,YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1454 "parser.tab.cc"
    break;

  case 87:
#line 231 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1460 "parser.tab.cc"
    break;

  case 88:
#line 232 "parser.y"
                         {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::Equal,YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1466 "parser.tab.cc"
    break;

  case 89:
#line 233 "parser.y"
                         {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::NotEqual,YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1472 "parser.tab.cc"
    break;

  case 90:
#line 237 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1478 "parser.tab.cc"
    break;

  case 91:
#line 238 "parser.y"
                           {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::LAnd,YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1484 "parser.tab.cc"
    break;

  case 92:
#line 242 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1490 "parser.tab.cc"
    break;

  case 93:
#line 243 "parser.y"
                           {yylhs.value.as < ASTPtr > ()=ctx.MakeAST<BinaryAST>(BinaryAST::Operator::LOr,YY_MOVE (yystack_[2].value.as < ASTPtr > ()),YY_MOVE (yystack_[0].value.as < ASTPtr > ()));}
#line 1496 "parser.tab.cc"
    break;

  case 94:
#line 247 "parser.y"
    { yylhs.value.as < ASTPtr > () = YY_MOVE (yystack_[0].value.as < ASTPtr > ()); }
#line 1502 "parser.tab.cc"
    break;

  case 95:
#line 251 "parser.y"
        {yylhs.value.as < std::string > ()=yyla.value.as<std::string>();}
#line 1508 "parser.tab.cc"
    break;


#line 1512 "parser.tab.cc"

            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        std::string msg = YY_("syntax error");
        error (yyla.location, YY_MOVE (msg));
      }


    yyerror_range[1].location = yyla.location;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.kind () == symbol_kind::S_YYEOF)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    YY_STACK_PRINT ();
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    // Pop stack until we find a state that shifts the error token.
    for (;;)
      {
        yyn = yypact_[+yystack_[0].state];
        if (!yy_pact_value_is_default_ (yyn))
          {
            yyn += symbol_kind::S_YYerror;
            if (0 <= yyn && yyn <= yylast_
                && yycheck_[yyn] == symbol_kind::S_YYerror)
              {
                yyn = yytable_[yyn];
                if (0 < yyn)
                  break;
              }
          }

        // Pop the current state because it cannot handle the error token.
        if (yystack_.size () == 1)
          YYABORT;

        yyerror_range[1].location = yystack_[0].location;
        yy_destroy_ ("Error: popping", yystack_[0]);
        yypop_ ();
        YY_STACK_PRINT ();
      }
    {
      stack_symbol_type error_token;

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

      // Shift the error token.
      error_token.state = state_type (yyn);
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    YY_STACK_PRINT ();
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  Parser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }


  const signed char Parser::yypact_ninf_ = -121;

  const signed char Parser::yytable_ninf_ = -1;

  const short
  Parser::yypact_[] =
  {
      62,  -121,  -121,  -121,    21,    13,  -121,  -121,    44,  -121,
    -121,    44,  -121,  -121,  -121,    70,    28,  -121,    76,    39,
    -121,    44,   188,   235,    14,    45,  -121,    44,   209,    47,
    -121,    54,  -121,    57,   235,   235,   235,   235,  -121,  -121,
    -121,  -121,  -121,  -121,     7,     5,    20,     5,    51,    48,
      44,  -121,   -12,   188,   235,  -121,    91,  -121,  -121,   209,
    -121,  -121,    68,    71,  -121,  -121,  -121,   235,   235,   235,
     235,   235,   235,   214,    78,  -121,   133,  -121,    80,    21,
      48,  -121,    56,  -121,  -121,    74,  -121,   188,  -121,  -121,
    -121,  -121,  -121,     7,     7,    85,  -121,  -121,    10,   235,
      92,   108,   118,   123,   230,  -121,  -121,  -121,    44,  -121,
    -121,  -121,   124,   125,   165,   119,  -121,  -121,  -121,   209,
    -121,  -121,  -121,   235,  -121,   120,   235,   235,  -121,  -121,
    -121,   128,  -121,   235,  -121,  -121,   126,  -121,  -121,  -121,
     127,     5,     8,    97,   138,   139,   130,  -121,   141,    78,
     193,   235,   235,   235,   235,   235,   235,   235,   235,   193,
    -121,   154,     5,     5,     5,     5,     8,     8,    97,   138,
    -121,   193,  -121
  };

  const signed char
  Parser::yydefact_[] =
  {
       0,     4,     7,     8,     0,     0,     2,     5,     0,     6,
       3,     0,     1,    95,    22,     0,    24,    10,     0,     0,
      21,     0,     0,     0,     0,    25,     9,     0,     0,     0,
      23,    24,    66,     0,     0,     0,     0,     0,    26,    28,
      64,    67,    65,    75,    79,    57,    59,    94,     0,     0,
       0,    35,     0,     0,     0,    11,     0,    12,    14,     0,
      29,    31,     0,     0,    70,    71,    72,     0,     0,     0,
       0,     0,     0,     0,    60,    19,     0,    33,    37,     0,
       0,    27,     0,    15,    17,     0,    13,     0,    30,    63,
      76,    77,    78,    80,    81,     0,    68,    73,     0,     0,
       0,     0,     0,     0,     0,    47,    40,    44,     0,    49,
      42,    45,     0,    64,     0,     0,    36,    34,    20,     0,
      16,    32,    61,     0,    69,     0,     0,     0,    54,    53,
      55,     0,    48,     0,    41,    43,    38,    18,    74,    62,
       0,    82,    87,    90,    92,    58,     0,    56,     0,    39,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      46,    50,    85,    86,    83,    84,    88,    89,    91,    93,
      52,     0,    51
  };

  const short
  Parser::yypgoto_[] =
  {
    -121,  -121,   170,  -121,     2,   146,   -48,  -121,   157,   -30,
    -121,   100,   -40,    66,  -120,   -18,    55,   -62,  -121,  -121,
      -8,    49,   -23,   -29,    24,    25,  -121,   -22,  -121,   167,
    -121,  -121,  -121,  -121,  -121,    53,  -121,    -1
  };

  const short
  Parser::yydefgoto_[] =
  {
      -1,     5,   107,     7,    50,    17,    57,     9,    14,    38,
      10,    51,   109,   110,   111,   112,   140,    40,    41,    42,
      43,    44,    45,   142,   143,   144,   145,    58,    18,    25,
      15,    85,    62,    52,   114,    74,    98,    46
  };

  const unsigned char
  Parser::yytable_[] =
  {
      47,    48,     8,    61,    39,    47,    11,    16,    84,    77,
      19,    86,    79,    12,   113,    39,    63,     2,     3,    80,
      31,   151,   152,    81,     2,     3,    19,    64,    65,    66,
     161,    47,    82,    47,   123,    39,    47,    70,    71,   170,
     117,   124,    67,    68,    69,    49,   153,   154,    72,    78,
      73,   172,   113,    22,    95,    97,    23,   121,    24,    90,
      91,    92,     1,    13,    28,     2,     3,    23,     4,    39,
      53,   137,    59,    54,    76,    54,    13,    32,   108,    22,
      75,   125,    23,    33,    60,   118,   131,    34,   113,    35,
      36,    37,    87,    20,    21,    88,    47,   113,   119,    26,
      27,   120,    89,   141,   141,   138,    99,    31,   115,   113,
      13,    32,   155,   156,   122,   148,   108,    56,    83,    93,
      94,    34,   126,    35,    36,    37,   166,   167,   162,   163,
     164,   165,   141,   141,   141,   141,     2,     3,   127,     4,
     100,   128,   101,   102,   103,   104,   129,   132,   136,   139,
     133,   147,    13,    32,    72,   157,   105,   158,   150,    76,
     106,   159,   171,    34,   160,    35,    36,    37,     2,     3,
       6,     4,   100,    55,   101,   102,   103,   104,    30,   116,
     135,   168,   146,   169,    13,    32,    29,     0,   105,   149,
       0,    76,   134,     0,     0,    34,     0,    35,    36,    37,
     100,     0,   101,   102,   103,   104,     0,    13,    32,     0,
       0,     0,    13,    32,    33,     0,   105,     0,    34,    76,
      35,    36,    37,    34,     0,    35,    36,    37,    13,    32,
       0,     0,     0,    13,    32,    56,     0,     0,     0,    34,
       0,    35,    36,    37,    34,    96,    35,    36,    37,    13,
      32,     0,     0,   130,    13,    32,     0,     0,     0,     0,
      34,     0,    35,    36,    37,    34,     0,    35,    36,    37
  };

  const short
  Parser::yycheck_[] =
  {
      23,    23,     0,    33,    22,    28,     4,     8,    56,    49,
      11,    59,    24,     0,    76,    33,    34,     3,     4,    31,
      21,    13,    14,    53,     3,     4,    27,    35,    36,    37,
     150,    54,    54,    56,    24,    53,    59,    32,    33,   159,
      80,    31,    35,    36,    37,    31,    38,    39,    28,    50,
      30,   171,   114,    25,    72,    73,    28,    87,    30,    67,
      68,    69,     0,    19,    25,     3,     4,    28,     6,    87,
      25,   119,    25,    28,    26,    28,    19,    20,    76,    25,
      29,    99,    28,    26,    27,    29,   104,    30,   150,    32,
      33,    34,    24,    23,    24,    27,   119,   159,    24,    23,
      24,    27,    31,   126,   127,   123,    28,   108,    28,   171,
      19,    20,    15,    16,    29,   133,   114,    26,    27,    70,
      71,    30,    30,    32,    33,    34,   155,   156,   151,   152,
     153,   154,   155,   156,   157,   158,     3,     4,    30,     6,
       7,    23,     9,    10,    11,    12,    23,    23,    29,    29,
      25,    23,    19,    20,    28,    17,    23,    18,    31,    26,
      27,    31,     8,    30,    23,    32,    33,    34,     3,     4,
       0,     6,     7,    27,     9,    10,    11,    12,    21,    79,
     114,   157,   127,   158,    19,    20,    19,    -1,    23,   136,
      -1,    26,    27,    -1,    -1,    30,    -1,    32,    33,    34,
       7,    -1,     9,    10,    11,    12,    -1,    19,    20,    -1,
      -1,    -1,    19,    20,    26,    -1,    23,    -1,    30,    26,
      32,    33,    34,    30,    -1,    32,    33,    34,    19,    20,
      -1,    -1,    -1,    19,    20,    26,    -1,    -1,    -1,    30,
      -1,    32,    33,    34,    30,    31,    32,    33,    34,    19,
      20,    -1,    -1,    23,    19,    20,    -1,    -1,    -1,    -1,
      30,    -1,    32,    33,    34,    30,    -1,    32,    33,    34
  };

  const signed char
  Parser::yystos_[] =
  {
       0,     0,     3,     4,     6,    41,    42,    43,    44,    47,
      50,    44,     0,    19,    48,    70,    77,    45,    68,    77,
      23,    24,    25,    28,    30,    69,    23,    24,    25,    69,
      48,    77,    20,    26,    30,    32,    33,    34,    49,    55,
      57,    58,    59,    60,    61,    62,    77,    62,    67,    31,
      44,    51,    73,    25,    28,    45,    26,    46,    67,    25,
      27,    49,    72,    55,    60,    60,    60,    35,    36,    37,
      32,    33,    28,    30,    75,    29,    26,    52,    77,    24,
      31,    49,    67,    27,    46,    71,    46,    24,    27,    31,
      60,    60,    60,    61,    61,    55,    31,    55,    76,    28,
       7,     9,    10,    11,    12,    23,    27,    42,    44,    52,
      53,    54,    55,    57,    74,    28,    51,    52,    29,    24,
      27,    49,    29,    24,    31,    55,    30,    30,    23,    23,
      23,    55,    23,    25,    27,    53,    29,    46,    55,    29,
      56,    62,    63,    64,    65,    66,    56,    23,    55,    75,
      31,    13,    14,    38,    39,    15,    16,    17,    18,    31,
      23,    54,    62,    62,    62,    62,    63,    63,    64,    65,
      54,     8,    54
  };

  const signed char
  Parser::yyr1_[] =
  {
       0,    40,    41,    41,    41,    42,    42,    44,    44,    43,
      68,    68,    45,    45,    46,    46,    46,    71,    71,    69,
      69,    47,    70,    70,    48,    48,    48,    48,    49,    49,
      49,    72,    72,    50,    50,    73,    73,    51,    51,    51,
      52,    52,    74,    74,    53,    53,    54,    54,    54,    54,
      54,    54,    54,    54,    54,    54,    54,    55,    56,    57,
      57,    75,    75,    58,    58,    58,    59,    60,    60,    60,
      60,    60,    60,    76,    76,    61,    61,    61,    61,    62,
      62,    62,    63,    63,    63,    63,    63,    64,    64,    64,
      65,    65,    66,    66,    67,    77
  };

  const signed char
  Parser::yyr2_[] =
  {
       0,     2,     1,     1,     1,     1,     1,     1,     1,     4,
       1,     3,     3,     4,     1,     2,     3,     1,     3,     3,
       4,     3,     1,     3,     1,     2,     3,     4,     1,     2,
       3,     1,     3,     5,     6,     1,     3,     2,     4,     5,
       2,     3,     1,     2,     1,     1,     4,     1,     2,     1,
       5,     7,     5,     2,     2,     2,     3,     1,     1,     1,
       2,     3,     4,     3,     1,     1,     1,     1,     3,     4,
       2,     2,     2,     1,     3,     1,     3,     3,     3,     1,
       3,     3,     1,     3,     3,     3,     3,     1,     3,     3,
       1,     3,     1,     3,     1,     1
  };


#if YYDEBUG
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
  const char*
  const Parser::yytname_[] =
  {
  "\"end of file\"", "error", "\"invalid token\"", "VOID", "INT", "CHAR",
  "CONST", "IF", "ELSE", "WHILE", "CONTINUE", "BREAK", "RETURN", "LE_OP",
  "GE_OP", "EQ_OP", "NE_OP", "AND_OP", "OR_OP", "IDENT", "INT_CONST",
  "CHAR_CONST", "STRING_CONST", "';'", "','", "'='", "'{'", "'}'", "'['",
  "']'", "'('", "')'", "'+'", "'-'", "'!'", "'*'", "'/'", "'%'", "'<'",
  "'>'", "$accept", "comp_unit", "decl", "const_decl", "btype",
  "const_def", "const_init_val", "var_decl", "var_def", "init_val",
  "func_def", "func_fparam", "block", "block_item", "stmt", "exp", "cond",
  "lval", "primary_exp", "number", "unary_exp", "mul_exp", "add_exp",
  "rel_exp", "eq_exp", "land_exp", "lor_exp", "const_exp", "const_defs",
  "arr_const_exps", "var_defs", "const_init_vals", "init_vals",
  "func_fparams", "block_items", "arr_exps", "func_rprarms", "ident", YY_NULLPTR
  };
#endif


#if YYDEBUG
  const unsigned char
  Parser::yyrline_[] =
  {
       0,    50,    50,    51,    52,    56,    57,    61,    62,    66,
      70,    71,    75,    76,    80,    81,    82,    86,    87,    91,
      92,    96,   100,   101,   105,   106,   107,   108,   112,   113,
     114,   118,   119,   123,   124,   129,   130,   134,   135,   136,
     139,   140,   144,   145,   149,   150,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,   164,   168,   172,   176,
     177,   181,   182,   186,   187,   188,   192,   196,   197,   198,
     199,   200,   201,   205,   206,   210,   211,   212,   213,   217,
     218,   219,   223,   224,   225,   226,   227,   231,   232,   233,
     237,   238,   242,   243,   247,   251
  };

  void
  Parser::yy_stack_print_ () const
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << int (i->state);
    *yycdebug_ << '\n';
  }

  void
  Parser::yy_reduce_print_ (int yyrule) const
  {
    int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG

  Parser::symbol_kind_type
  Parser::yytranslate_ (int t)
  {
    // YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to
    // TOKEN-NUM as returned by yylex.
    static
    const signed char
    translate_table[] =
    {
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    34,     2,     2,     2,    37,     2,     2,
      30,    31,    35,    32,    24,    33,     2,    36,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    23,
      38,    25,    39,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    28,     2,    29,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    26,     2,    27,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22
    };
    const int user_token_number_max_ = 277;

    if (t <= 0)
      return symbol_kind::S_YYEOF;
    else if (t <= user_token_number_max_)
      return YY_CAST (symbol_kind_type, translate_table[t]);
    else
      return symbol_kind::S_YYUNDEF;
  }

} // yy
#line 1990 "parser.tab.cc"

#line 254 "parser.y"


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

namespace yy{

  void Parser::error(location const &loc,const std::string& s){
    std::cerr<<yytext<<std::endl;
    std::cerr<<"error at "<<loc.begin<<":"<<s<<std::endl;
  }
}
