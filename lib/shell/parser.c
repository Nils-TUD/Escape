
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* "%code top" blocks.  */

/* Line 171 of yacc.c  */
#line 1 "script.y"

	#define YYDEBUG 1
	#include <stdio.h>
	int yylex (void);
	void yyerror(char const *,...);



/* Line 171 of yacc.c  */
#line 80 "parser.c"


/* Copy the first part of user declarations.  */


/* Line 189 of yacc.c  */
#line 87 "parser.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

/* "%code requires" blocks.  */

/* Line 209 of yacc.c  */
#line 8 "script.y"

	#include "ast/node.h"
	#include "ast/assignexpr.h"
	#include "ast/binaryopexpr.h"
	#include "ast/cmpexpr.h"
	#include "ast/conststrexpr.h"
	#include "ast/ifstmt.h"
	#include "ast/intexpr.h"
	#include "ast/stmtlist.h"
	#include "ast/unaryopexpr.h"
	#include "ast/varexpr.h"
	#include "ast/command.h"
	#include "ast/cmdexprlist.h"
	#include "ast/subcmd.h"
	#include "ast/redirfd.h"
	#include "ast/redirfile.h"
	#include "ast/forstmt.h"
	#include "ast/exprstmt.h"
	#include "ast/dstrexpr.h"
	#include "ast/whilestmt.h"
	#include "ast/functionstmt.h"
	#include "exec/env.h"
	#include "mem.h"
	#include "shell.h"



/* Line 209 of yacc.c  */
#line 139 "parser.c"

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_IF = 258,
     T_THEN = 259,
     T_ELSE = 260,
     T_FI = 261,
     T_FOR = 262,
     T_DO = 263,
     T_DONE = 264,
     T_WHILE = 265,
     T_FUNCTION = 266,
     T_BEGIN = 267,
     T_END = 268,
     T_NUMBER = 269,
     T_STRING = 270,
     T_STRING_SCONST = 271,
     T_VAR = 272,
     T_ERR2OUT = 273,
     T_OUT2ERR = 274,
     T_APPEND = 275,
     T_ERR2FILE = 276,
     T_OUT2FILE = 277,
     T_ASSIGN = 278,
     T_NEQ = 279,
     T_EQ = 280,
     T_GEQ = 281,
     T_LEQ = 282,
     T_SUB = 283,
     T_ADD = 284,
     T_MOD = 285,
     T_DIV = 286,
     T_MUL = 287,
     T_NEG = 288,
     T_DEC = 289,
     T_INC = 290,
     T_POW = 291
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 33 "script.y"

	int intval;
	char *strval;
	sASTNode *node;



/* Line 214 of yacc.c  */
#line 200 "parser.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 212 "parser.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  47
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   324

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  46
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  20
/* YYNRULES -- Number of rules.  */
#define YYNRULES  78
/* YYNRULES -- Number of states.  */
#define YYNSTATES  160

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   291

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    42,     2,     2,     2,    44,     2,
      40,    41,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    39,
      24,     2,    23,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,    43,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    45,     2,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    21,    22,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     6,     8,    11,    13,    17,    23,
      31,    41,    53,    61,    63,    65,    68,    69,    71,    73,
      77,    79,    81,    85,    87,    89,    93,    97,   101,   105,
     109,   113,   116,   119,   122,   125,   128,   132,   136,   140,
     144,   148,   152,   156,   160,   164,   166,   168,   170,   174,
     176,   180,   184,   186,   189,   191,   194,   200,   208,   210,
     212,   214,   218,   220,   224,   226,   229,   235,   243,   245,
     247,   248,   251,   252,   255,   256,   259,   262,   265
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      47,     0,    -1,    48,    -1,    -1,    49,    -1,    49,    39,
      -1,    50,    -1,    49,    39,    50,    -1,    11,    15,    12,
      48,    13,    -1,     3,    40,    53,    41,     4,    48,     6,
      -1,     3,    40,    53,    41,     4,    48,     5,    48,     6,
      -1,     7,    40,    53,    39,    53,    39,    53,    41,     8,
      48,     9,    -1,    10,    40,    53,    41,     8,    48,     9,
      -1,    54,    -1,    57,    -1,    51,    52,    -1,    -1,    15,
      -1,    17,    -1,    40,    53,    41,    -1,    14,    -1,    16,
      -1,    42,    51,    42,    -1,    17,    -1,    54,    -1,    53,
      31,    53,    -1,    53,    30,    53,    -1,    53,    34,    53,
      -1,    53,    33,    53,    -1,    53,    32,    53,    -1,    53,
      38,    53,    -1,    30,    53,    -1,    37,    17,    -1,    36,
      17,    -1,    17,    37,    -1,    17,    36,    -1,    53,    24,
      53,    -1,    53,    23,    53,    -1,    53,    29,    53,    -1,
      53,    28,    53,    -1,    53,    27,    53,    -1,    53,    26,
      53,    -1,    43,    61,    43,    -1,    40,    53,    41,    -1,
      17,    25,    53,    -1,    14,    -1,    15,    -1,    16,    -1,
      42,    51,    42,    -1,    17,    -1,    43,    61,    43,    -1,
      40,    53,    41,    -1,    55,    -1,    56,    55,    -1,    58,
      -1,    58,    44,    -1,    56,    63,    65,    64,    62,    -1,
      58,    45,    56,    63,    65,    64,    62,    -1,    14,    -1,
      15,    -1,    16,    -1,    42,    51,    42,    -1,    17,    -1,
      40,    53,    41,    -1,    59,    -1,    60,    59,    -1,    60,
      63,    65,    64,    62,    -1,    61,    45,    60,    63,    65,
      64,    62,    -1,    18,    -1,    19,    -1,    -1,    24,    59,
      -1,    -1,    21,    59,    -1,    -1,    23,    59,    -1,    22,
      59,    -1,    20,    59,    -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    79,    79,    86,    87,    88,    92,    93,    97,   100,
     103,   106,   109,   112,   115,   121,   122,   126,   127,   128,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,   149,   150,   151,
     152,   153,   154,   155,   159,   163,   164,   165,   166,   167,
     168,   169,   173,   174,   178,   179,   183,   187,   197,   198,
     199,   200,   201,   202,   206,   207,   211,   215,   224,   225,
     226,   230,   231,   235,   236,   240,   241,   242,   243
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "T_IF", "T_THEN", "T_ELSE", "T_FI",
  "T_FOR", "T_DO", "T_DONE", "T_WHILE", "T_FUNCTION", "T_BEGIN", "T_END",
  "T_NUMBER", "T_STRING", "T_STRING_SCONST", "T_VAR", "T_ERR2OUT",
  "T_OUT2ERR", "T_APPEND", "T_ERR2FILE", "T_OUT2FILE", "'>'", "'<'",
  "T_ASSIGN", "T_NEQ", "T_EQ", "T_GEQ", "T_LEQ", "T_SUB", "T_ADD", "T_MOD",
  "T_DIV", "T_MUL", "T_NEG", "T_DEC", "T_INC", "T_POW", "';'", "'('",
  "')'", "'\"'", "'`'", "'&'", "'|'", "$accept", "start", "stmtlist",
  "stmtlistr", "stmt", "strlist", "strcomp", "expr", "assignstmt",
  "cmdexpr", "cmdexprlist", "cmd", "subcmd", "nestedcmdexpr",
  "nestedcmdexprlist", "nestedsubcmd", "cmdredirfd", "cmdredirin",
  "cmdredirerr", "cmdredirout", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,    62,    60,   278,   279,   280,   281,   282,
     283,   284,   285,   286,   287,   288,   289,   290,   291,    59,
      40,    41,    34,    96,    38,   124
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    46,    47,    48,    48,    48,    49,    49,    50,    50,
      50,    50,    50,    50,    50,    51,    51,    52,    52,    52,
      53,    53,    53,    53,    53,    53,    53,    53,    53,    53,
      53,    53,    53,    53,    53,    53,    53,    53,    53,    53,
      53,    53,    53,    53,    54,    55,    55,    55,    55,    55,
      55,    55,    56,    56,    57,    57,    58,    58,    59,    59,
      59,    59,    59,    59,    60,    60,    61,    61,    62,    62,
      62,    63,    63,    64,    64,    65,    65,    65,    65
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     1,     2,     1,     3,     5,     7,
       9,    11,     7,     1,     1,     2,     0,     1,     1,     3,
       1,     1,     3,     1,     1,     3,     3,     3,     3,     3,
       3,     2,     2,     2,     2,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     1,     1,     1,     3,     1,
       3,     3,     1,     2,     1,     2,     5,     7,     1,     1,
       1,     3,     1,     3,     1,     2,     5,     7,     1,     1,
       0,     2,     0,     2,     0,     2,     2,     2,     0
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     0,     0,     0,    45,    46,    47,    49,     0,
      16,     0,     0,     2,     4,     6,    13,    52,    72,    14,
      54,     0,     0,     0,     0,     0,    20,    21,    23,     0,
       0,     0,     0,    16,     0,     0,    24,     0,    58,    59,
      60,    62,     0,    16,    64,    72,     0,     1,     5,    49,
       0,    53,    78,    55,     0,     0,     0,     0,     3,    44,
      35,    34,    31,    33,    32,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      51,    17,    18,     0,    48,    15,     0,     0,    65,    78,
      50,     0,     7,    71,     0,     0,     0,    74,    72,     0,
       0,     0,     0,    43,    22,    42,    37,    36,    41,    40,
      39,    38,    26,    25,    29,    28,    27,    30,     0,    63,
      61,    74,    72,    77,    76,    75,     0,    70,    78,     3,
       0,     3,     8,    19,    70,    78,    73,    68,    69,    56,
      74,     0,     0,     0,    66,    74,    70,     3,     9,     0,
      12,    70,    57,     0,     0,    67,    10,     3,     0,    11
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    12,    13,    14,    15,    37,    85,    35,    36,    17,
      18,    19,    20,    44,    45,    46,   139,    52,   127,    97
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -120
static const yytype_int16 yypact[] =
{
      14,   -35,   -18,   -14,    25,  -120,  -120,  -120,    47,    70,
    -120,   111,    44,  -120,    37,  -120,  -120,  -120,    -6,  -120,
      36,    70,    70,    70,    73,    70,  -120,  -120,    65,    70,
      75,    81,    70,  -120,   111,   140,  -120,    94,  -120,  -120,
    -120,  -120,    70,  -120,  -120,   100,     4,  -120,    14,  -120,
     111,  -120,    19,  -120,    80,   156,   252,   172,    14,   286,
    -120,  -120,    61,  -120,  -120,   188,   104,    28,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
    -120,  -120,  -120,    70,  -120,  -120,   204,   120,  -120,    19,
    -120,   111,  -120,  -120,   111,   111,   111,    87,    -6,   114,
      70,   122,   119,  -120,  -120,  -120,   286,   286,   286,   286,
     286,   286,   116,   116,    61,    61,    61,    61,   220,  -120,
    -120,    87,   100,  -120,  -120,  -120,   111,    86,    19,    14,
     269,    14,  -120,  -120,    86,    19,  -120,  -120,  -120,  -120,
      87,    83,    70,   129,  -120,    87,    86,    14,  -120,   236,
    -120,    86,  -120,   133,   135,  -120,  -120,    14,   132,  -120
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
    -120,  -120,   -54,  -120,    97,     2,  -120,    -9,     0,   -15,
      98,  -120,  -120,   -44,    64,   124,  -119,   -43,  -102,   -82
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_int16 yytable[] =
{
      16,    88,    89,    51,   102,    21,    93,   121,     5,     6,
       7,    49,    55,    56,    57,   144,    59,     1,    50,   134,
      62,     2,    22,    65,     3,     4,    23,   152,     5,     6,
       7,     8,   155,    86,     9,    66,    10,    11,   146,    94,
      24,    95,    96,   151,    47,    87,   140,    90,    16,    91,
     123,   124,   125,   145,     9,   128,    10,    11,    16,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   105,    25,    91,   118,   141,    48,   143,    88,   135,
      53,    54,   136,    51,    26,    58,    27,    28,   147,   148,
      25,   130,    63,   153,     5,     6,     7,    49,    64,    79,
      29,    60,    61,   158,   137,   138,    30,    31,   126,    81,
      32,    82,    33,    34,    38,    39,    40,    41,   129,    81,
       9,    82,    10,    11,    50,    38,    39,    40,    41,    16,
     131,    16,   132,   149,    83,    81,    84,    82,   150,   156,
      42,   159,    43,   157,    83,    92,   104,    16,    76,    77,
      78,    42,    98,    43,    79,   122,     0,    16,    67,     0,
      83,     0,   120,    68,    69,     0,    70,    71,    72,    73,
      74,    75,    76,    77,    78,     0,     0,     0,    79,    68,
      69,    80,    70,    71,    72,    73,    74,    75,    76,    77,
      78,     0,     0,     0,    79,    68,    69,    99,    70,    71,
      72,    73,    74,    75,    76,    77,    78,     0,     0,     0,
      79,    68,    69,   101,    70,    71,    72,    73,    74,    75,
      76,    77,    78,     0,     0,     0,    79,    68,    69,   103,
      70,    71,    72,    73,    74,    75,    76,    77,    78,     0,
       0,     0,    79,    68,    69,   119,    70,    71,    72,    73,
      74,    75,    76,    77,    78,     0,     0,     0,    79,    68,
      69,   133,    70,    71,    72,    73,    74,    75,    76,    77,
      78,     0,     0,     0,    79,    68,    69,   154,    70,    71,
      72,    73,    74,    75,    76,    77,    78,     0,     0,     0,
      79,   100,    68,    69,     0,    70,    71,    72,    73,    74,
      75,    76,    77,    78,     0,     0,     0,    79,   142,    -1,
      -1,     0,    -1,    -1,    -1,    -1,    74,    75,    76,    77,
      78,     0,     0,     0,    79
};

static const yytype_int16 yycheck[] =
{
       0,    45,    45,    18,    58,    40,    50,    89,    14,    15,
      16,    17,    21,    22,    23,   134,    25,     3,    24,   121,
      29,     7,    40,    32,    10,    11,    40,   146,    14,    15,
      16,    17,   151,    42,    40,    33,    42,    43,   140,    20,
      15,    22,    23,   145,     0,    43,   128,    43,    48,    45,
      94,    95,    96,   135,    40,    98,    42,    43,    58,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    43,    25,    45,    83,   129,    39,   131,   122,   122,
      44,    45,   126,    98,    14,    12,    16,    17,     5,     6,
      25,   100,    17,   147,    14,    15,    16,    17,    17,    38,
      30,    36,    37,   157,    18,    19,    36,    37,    21,    15,
      40,    17,    42,    43,    14,    15,    16,    17,     4,    15,
      40,    17,    42,    43,    24,    14,    15,    16,    17,   129,
       8,   131,    13,   142,    40,    15,    42,    17,     9,     6,
      40,     9,    42,     8,    40,    48,    42,   147,    32,    33,
      34,    40,    54,    42,    38,    91,    -1,   157,    34,    -1,
      40,    -1,    42,    23,    24,    -1,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,    -1,    -1,    38,    23,
      24,    41,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    -1,    -1,    38,    23,    24,    41,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    -1,    -1,
      38,    23,    24,    41,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    -1,    -1,    -1,    38,    23,    24,    41,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
      -1,    -1,    38,    23,    24,    41,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,    -1,    -1,    38,    23,
      24,    41,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    -1,    -1,    38,    23,    24,    41,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    -1,    -1,
      38,    39,    23,    24,    -1,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    -1,    -1,    -1,    38,    39,    23,
      24,    -1,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    -1,    -1,    38
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     7,    10,    11,    14,    15,    16,    17,    40,
      42,    43,    47,    48,    49,    50,    54,    55,    56,    57,
      58,    40,    40,    40,    15,    25,    14,    16,    17,    30,
      36,    37,    40,    42,    43,    53,    54,    51,    14,    15,
      16,    17,    40,    42,    59,    60,    61,     0,    39,    17,
      24,    55,    63,    44,    45,    53,    53,    53,    12,    53,
      36,    37,    53,    17,    17,    53,    51,    61,    23,    24,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    38,
      41,    15,    17,    40,    42,    52,    53,    51,    59,    63,
      43,    45,    50,    59,    20,    22,    23,    65,    56,    41,
      39,    41,    48,    41,    42,    43,    53,    53,    53,    53,
      53,    53,    53,    53,    53,    53,    53,    53,    53,    41,
      42,    65,    60,    59,    59,    59,    21,    64,    63,     4,
      53,     8,    13,    41,    64,    63,    59,    18,    19,    62,
      65,    48,    39,    48,    62,    65,    64,     5,     6,    53,
       9,    64,    62,    48,    41,    62,     6,     8,    48,     9
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {
      case 15: /* "T_STRING" */

/* Line 1000 of yacc.c  */
#line 73 "script.y"
	{ free((yyvaluep->strval)); };

/* Line 1000 of yacc.c  */
#line 1275 "parser.c"
	break;
      case 16: /* "T_STRING_SCONST" */

/* Line 1000 of yacc.c  */
#line 73 "script.y"
	{ free((yyvaluep->strval)); };

/* Line 1000 of yacc.c  */
#line 1284 "parser.c"
	break;
      case 17: /* "T_VAR" */

/* Line 1000 of yacc.c  */
#line 73 "script.y"
	{ free((yyvaluep->strval)); };

/* Line 1000 of yacc.c  */
#line 1293 "parser.c"
	break;
      case 48: /* "stmtlist" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1302 "parser.c"
	break;
      case 49: /* "stmtlistr" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1311 "parser.c"
	break;
      case 50: /* "stmt" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1320 "parser.c"
	break;
      case 51: /* "strlist" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1329 "parser.c"
	break;
      case 52: /* "strcomp" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1338 "parser.c"
	break;
      case 53: /* "expr" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1347 "parser.c"
	break;
      case 54: /* "assignstmt" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1356 "parser.c"
	break;
      case 55: /* "cmdexpr" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1365 "parser.c"
	break;
      case 56: /* "cmdexprlist" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1374 "parser.c"
	break;
      case 57: /* "cmd" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1383 "parser.c"
	break;
      case 58: /* "subcmd" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1392 "parser.c"
	break;
      case 59: /* "nestedcmdexpr" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1401 "parser.c"
	break;
      case 60: /* "nestedcmdexprlist" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1410 "parser.c"
	break;
      case 61: /* "nestedsubcmd" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1419 "parser.c"
	break;
      case 62: /* "cmdredirfd" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1428 "parser.c"
	break;
      case 63: /* "cmdredirin" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1437 "parser.c"
	break;
      case 64: /* "cmdredirerr" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1446 "parser.c"
	break;
      case 65: /* "cmdredirout" */

/* Line 1000 of yacc.c  */
#line 74 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1455 "parser.c"
	break;

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1455 of yacc.c  */
#line 79 "script.y"
    {
				ast_execute(curEnv,(yyvsp[(1) - (1)].node));
				ast_destroy((yyvsp[(1) - (1)].node));
			;}
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 86 "script.y"
    { (yyval.node) = ast_createStmtList(); ;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 87 "script.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); ;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 88 "script.y"
    { (yyval.node) = (yyvsp[(1) - (2)].node); ;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 92 "script.y"
    { (yyval.node) = ast_createStmtList(); ast_addStmt((yyval.node),(yyvsp[(1) - (1)].node)); ;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 93 "script.y"
    { (yyval.node) = ast_addStmt((yyvsp[(1) - (3)].node),(yyvsp[(3) - (3)].node)); ;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 97 "script.y"
    {
				(yyval.node) = ast_createFunctionStmt((yyvsp[(2) - (5)].strval),(yyvsp[(4) - (5)].node));
			;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 100 "script.y"
    {
				(yyval.node) = ast_createIfStmt((yyvsp[(3) - (7)].node),(yyvsp[(6) - (7)].node),NULL);
			;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 103 "script.y"
    {
				(yyval.node) = ast_createIfStmt((yyvsp[(3) - (9)].node),(yyvsp[(6) - (9)].node),(yyvsp[(8) - (9)].node));
			;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 106 "script.y"
    {
				(yyval.node) = ast_createForStmt((yyvsp[(3) - (11)].node),(yyvsp[(5) - (11)].node),(yyvsp[(7) - (11)].node),(yyvsp[(10) - (11)].node));
			;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 109 "script.y"
    {
				(yyval.node) = ast_createWhileStmt((yyvsp[(3) - (7)].node),(yyvsp[(6) - (7)].node));
			;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 112 "script.y"
    {
				(yyval.node) = (yyvsp[(1) - (1)].node);
			;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 115 "script.y"
    {
				(yyval.node) = (yyvsp[(1) - (1)].node);
			;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 121 "script.y"
    { (yyval.node) = (yyvsp[(1) - (2)].node); ast_addDStrComp((yyvsp[(1) - (2)].node),(yyvsp[(2) - (2)].node)); ;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 122 "script.y"
    { (yyval.node) = ast_createDStrExpr(); ;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 126 "script.y"
    { (yyval.node) = ast_createConstStrExpr((yyvsp[(1) - (1)].strval),false); ;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 127 "script.y"
    { (yyval.node) = ast_createVarExpr((yyvsp[(1) - (1)].strval)); ;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 128 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 132 "script.y"
    { (yyval.node) = ast_createIntExpr((yyvsp[(1) - (1)].intval)); ;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 133 "script.y"
    { (yyval.node) = ast_createConstStrExpr((yyvsp[(1) - (1)].strval),true); ;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 134 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 135 "script.y"
    { (yyval.node) = ast_createVarExpr((yyvsp[(1) - (1)].strval)); ;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 136 "script.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 137 "script.y"
    { (yyval.node) = ast_createBinOpExpr((yyvsp[(1) - (3)].node),'+',(yyvsp[(3) - (3)].node)); ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 138 "script.y"
    { (yyval.node) = ast_createBinOpExpr((yyvsp[(1) - (3)].node),'-',(yyvsp[(3) - (3)].node)); ;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 139 "script.y"
    { (yyval.node) = ast_createBinOpExpr((yyvsp[(1) - (3)].node),'*',(yyvsp[(3) - (3)].node)); ;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 140 "script.y"
    { (yyval.node) = ast_createBinOpExpr((yyvsp[(1) - (3)].node),'/',(yyvsp[(3) - (3)].node)); ;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 141 "script.y"
    { (yyval.node) = ast_createBinOpExpr((yyvsp[(1) - (3)].node),'%',(yyvsp[(3) - (3)].node)); ;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 142 "script.y"
    { (yyval.node) = ast_createBinOpExpr((yyvsp[(1) - (3)].node),'^',(yyvsp[(3) - (3)].node)); ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 143 "script.y"
    { (yyval.node) = ast_createUnaryOpExpr((yyvsp[(2) - (2)].node),UN_OP_NEG); ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 144 "script.y"
    { (yyval.node) = ast_createUnaryOpExpr(ast_createVarExpr((yyvsp[(2) - (2)].strval)),UN_OP_PREINC); ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 145 "script.y"
    { (yyval.node) = ast_createUnaryOpExpr(ast_createVarExpr((yyvsp[(2) - (2)].strval)),UN_OP_PREDEC); ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 146 "script.y"
    { (yyval.node) = ast_createUnaryOpExpr(ast_createVarExpr((yyvsp[(1) - (2)].strval)),UN_OP_POSTINC); ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 147 "script.y"
    { (yyval.node) = ast_createUnaryOpExpr(ast_createVarExpr((yyvsp[(1) - (2)].strval)),UN_OP_POSTDEC); ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 148 "script.y"
    { (yyval.node) = ast_createCmpExpr((yyvsp[(1) - (3)].node),CMP_OP_LT,(yyvsp[(3) - (3)].node)); ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 149 "script.y"
    { (yyval.node) = ast_createCmpExpr((yyvsp[(1) - (3)].node),CMP_OP_GT,(yyvsp[(3) - (3)].node)); ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 150 "script.y"
    { (yyval.node) = ast_createCmpExpr((yyvsp[(1) - (3)].node),CMP_OP_LEQ,(yyvsp[(3) - (3)].node)); ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 151 "script.y"
    { (yyval.node) = ast_createCmpExpr((yyvsp[(1) - (3)].node),CMP_OP_GEQ,(yyvsp[(3) - (3)].node)); ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 152 "script.y"
    { (yyval.node) = ast_createCmpExpr((yyvsp[(1) - (3)].node),CMP_OP_EQ,(yyvsp[(3) - (3)].node)); ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 153 "script.y"
    { (yyval.node) = ast_createCmpExpr((yyvsp[(1) - (3)].node),CMP_OP_NEQ,(yyvsp[(3) - (3)].node)); ;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 154 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ast_setRetOutput((yyvsp[(2) - (3)].node),true); ;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 155 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 159 "script.y"
    { (yyval.node) = ast_createAssignExpr(ast_createVarExpr((yyvsp[(1) - (3)].strval)),(yyvsp[(3) - (3)].node)); ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 163 "script.y"
    { (yyval.node) = ast_createIntExpr((yyvsp[(1) - (1)].intval)); ;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 164 "script.y"
    { (yyval.node) = ast_createConstStrExpr((yyvsp[(1) - (1)].strval),false); ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 165 "script.y"
    { (yyval.node) = ast_createConstStrExpr((yyvsp[(1) - (1)].strval),true); ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 166 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 167 "script.y"
    { (yyval.node) = ast_createVarExpr((yyvsp[(1) - (1)].strval)); ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 168 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ast_setRetOutput((yyvsp[(2) - (3)].node),true); ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 169 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 173 "script.y"
    { (yyval.node) = ast_createCmdExprList(); ast_addCmdExpr((yyval.node),(yyvsp[(1) - (1)].node)); ;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 174 "script.y"
    { (yyval.node) = (yyvsp[(1) - (2)].node); ast_addCmdExpr((yyvsp[(1) - (2)].node),(yyvsp[(2) - (2)].node)); ;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 178 "script.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); ;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 179 "script.y"
    { (yyval.node) = (yyvsp[(1) - (2)].node); ast_setRunInBG((yyvsp[(1) - (2)].node),true); ;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 183 "script.y"
    {
				(yyval.node) = ast_createCommand();
				ast_addSubCmd((yyval.node),ast_createSubCmd((yyvsp[(1) - (5)].node),(yyvsp[(5) - (5)].node),(yyvsp[(2) - (5)].node),(yyvsp[(3) - (5)].node),(yyvsp[(4) - (5)].node)));
			;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 187 "script.y"
    {
				(yyval.node) = (yyvsp[(1) - (7)].node);
				ast_addSubCmd((yyvsp[(1) - (7)].node),ast_createSubCmd((yyvsp[(3) - (7)].node),(yyvsp[(7) - (7)].node),(yyvsp[(4) - (7)].node),(yyvsp[(5) - (7)].node),(yyvsp[(6) - (7)].node)));
			;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 197 "script.y"
    { (yyval.node) = ast_createIntExpr((yyvsp[(1) - (1)].intval)); ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 198 "script.y"
    { (yyval.node) = ast_createConstStrExpr((yyvsp[(1) - (1)].strval),false); ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 199 "script.y"
    { (yyval.node) = ast_createConstStrExpr((yyvsp[(1) - (1)].strval),true); ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 200 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 201 "script.y"
    { (yyval.node) = ast_createVarExpr((yyvsp[(1) - (1)].strval)); ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 202 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 206 "script.y"
    { (yyval.node) = ast_createCmdExprList(); ast_addCmdExpr((yyval.node),(yyvsp[(1) - (1)].node)); ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 207 "script.y"
    { (yyval.node) = (yyvsp[(1) - (2)].node); ast_addCmdExpr((yyvsp[(1) - (2)].node),(yyvsp[(2) - (2)].node)); ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 211 "script.y"
    {
				(yyval.node) = ast_createCommand();
				ast_addSubCmd((yyval.node),ast_createSubCmd((yyvsp[(1) - (5)].node),(yyvsp[(5) - (5)].node),(yyvsp[(2) - (5)].node),(yyvsp[(3) - (5)].node),(yyvsp[(4) - (5)].node)));
			;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 215 "script.y"
    {
				(yyval.node) = (yyvsp[(1) - (7)].node);
				ast_addSubCmd((yyvsp[(1) - (7)].node),ast_createSubCmd((yyvsp[(3) - (7)].node),(yyvsp[(7) - (7)].node),(yyvsp[(4) - (7)].node),(yyvsp[(5) - (7)].node),(yyvsp[(6) - (7)].node)));
			;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 224 "script.y"
    { (yyval.node) = ast_createRedirFd(REDIR_ERR2OUT); ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 225 "script.y"
    { (yyval.node) = ast_createRedirFd(REDIR_OUT2ERR); ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 226 "script.y"
    { (yyval.node) = ast_createRedirFd(REDIR_NO); ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 230 "script.y"
    { (yyval.node) = ast_createRedirFile((yyvsp[(2) - (2)].node),REDIR_INFILE); ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 231 "script.y"
    { (yyval.node) = ast_createRedirFile(NULL,REDIR_OUTCREATE); ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 235 "script.y"
    { (yyval.node) = ast_createRedirFile((yyvsp[(2) - (2)].node),REDIR_OUTCREATE); ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 236 "script.y"
    { (yyval.node) = ast_createRedirFile(NULL,REDIR_OUTCREATE); ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 240 "script.y"
    { (yyval.node) = ast_createRedirFile((yyvsp[(2) - (2)].node),REDIR_OUTCREATE); ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 241 "script.y"
    { (yyval.node) = ast_createRedirFile((yyvsp[(2) - (2)].node),REDIR_OUTCREATE); ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 242 "script.y"
    { (yyval.node) = ast_createRedirFile((yyvsp[(2) - (2)].node),REDIR_OUTAPPEND); ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 243 "script.y"
    { (yyval.node) = ast_createRedirFile(NULL,REDIR_OUTCREATE); ;}
    break;



/* Line 1455 of yacc.c  */
#line 2328 "parser.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
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

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 246 "script.y"


