
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
	#include "ast/exprlist.h"
	#include "ast/property.h"
	#include "exec/env.h"
	#include "mem.h"
	#include "shell.h"



/* Line 209 of yacc.c  */
#line 141 "parser.c"

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
#line 35 "script.y"

	int intval;
	char *strval;
	sASTNode *node;



/* Line 214 of yacc.c  */
#line 202 "parser.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 214 "parser.c"

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
#define YYFINAL  49
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   433

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  50
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  22
/* YYNRULES -- Number of rules.  */
#define YYNRULES  87
/* YYNRULES -- Number of states.  */
#define YYNSTATES  180

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
       2,     2,     2,     2,    43,     2,     2,     2,    48,     2,
      41,    42,     2,     2,    47,     2,    39,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    40,
      24,     2,    23,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    44,     2,    45,     2,     2,    46,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    49,     2,     2,     2,     2,     2,
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
      77,    79,    81,    85,    87,    92,    96,    98,   102,   106,
     110,   114,   118,   122,   125,   128,   131,   134,   137,   141,
     145,   149,   153,   157,   161,   165,   169,   173,   175,   176,
     180,   182,   186,   192,   199,   201,   203,   205,   209,   211,
     215,   219,   221,   224,   226,   229,   235,   243,   245,   247,
     249,   253,   255,   259,   261,   264,   270,   278,   280,   282,
     283,   286,   287,   290,   291,   294,   297,   300
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      51,     0,    -1,    52,    -1,    -1,    53,    -1,    53,    40,
      -1,    54,    -1,    53,    40,    54,    -1,    11,    15,    12,
      52,    13,    -1,     3,    41,    57,    42,     4,    52,     6,
      -1,     3,    41,    57,    42,     4,    52,     5,    52,     6,
      -1,     7,    41,    57,    40,    57,    40,    57,    42,     8,
      52,     9,    -1,    10,    41,    57,    42,     8,    52,     9,
      -1,    60,    -1,    63,    -1,    55,    56,    -1,    -1,    15,
      -1,    17,    -1,    41,    57,    42,    -1,    14,    -1,    16,
      -1,    43,    55,    43,    -1,    17,    -1,    17,    44,    57,
      45,    -1,    44,    58,    45,    -1,    60,    -1,    57,    31,
      57,    -1,    57,    30,    57,    -1,    57,    34,    57,    -1,
      57,    33,    57,    -1,    57,    32,    57,    -1,    57,    38,
      57,    -1,    30,    57,    -1,    37,    17,    -1,    36,    17,
      -1,    17,    37,    -1,    17,    36,    -1,    57,    24,    57,
      -1,    57,    23,    57,    -1,    57,    29,    57,    -1,    57,
      28,    57,    -1,    57,    27,    57,    -1,    57,    26,    57,
      -1,    46,    67,    46,    -1,    41,    57,    42,    -1,    57,
      39,    15,    -1,    59,    -1,    -1,    59,    47,    57,    -1,
      57,    -1,    17,    25,    57,    -1,    17,    44,    45,    25,
      57,    -1,    17,    44,    57,    45,    25,    57,    -1,    14,
      -1,    15,    -1,    16,    -1,    43,    55,    43,    -1,    17,
      -1,    46,    67,    46,    -1,    41,    57,    42,    -1,    61,
      -1,    62,    61,    -1,    64,    -1,    64,    48,    -1,    62,
      69,    71,    70,    68,    -1,    64,    49,    62,    69,    71,
      70,    68,    -1,    14,    -1,    15,    -1,    16,    -1,    43,
      55,    43,    -1,    17,    -1,    41,    57,    42,    -1,    65,
      -1,    66,    65,    -1,    66,    69,    71,    70,    68,    -1,
      67,    49,    66,    69,    71,    70,    68,    -1,    18,    -1,
      19,    -1,    -1,    24,    65,    -1,    -1,    21,    65,    -1,
      -1,    23,    65,    -1,    22,    65,    -1,    20,    65,    -1,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    82,    82,    89,    90,    91,    95,    96,   100,   103,
     106,   109,   112,   115,   118,   124,   125,   129,   130,   131,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   166,   167,   171,
     172,   176,   179,   182,   188,   189,   190,   191,   192,   193,
     194,   198,   199,   203,   204,   208,   212,   222,   223,   224,
     225,   226,   227,   231,   232,   236,   240,   249,   250,   251,
     255,   256,   260,   261,   265,   266,   267,   268
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
  "T_DIV", "T_MUL", "T_NEG", "T_DEC", "T_INC", "T_POW", "'.'", "';'",
  "'('", "')'", "'\"'", "'['", "']'", "'`'", "','", "'&'", "'|'",
  "$accept", "start", "stmtlist", "stmtlistr", "stmt", "strlist",
  "strcomp", "expr", "exprlist", "neexprlist", "assignstmt", "cmdexpr",
  "cmdexprlist", "cmd", "subcmd", "nestedcmdexpr", "nestedcmdexprlist",
  "nestedsubcmd", "cmdredirfd", "cmdredirin", "cmdredirerr", "cmdredirout", 0
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
     283,   284,   285,   286,   287,   288,   289,   290,   291,    46,
      59,    40,    41,    34,    91,    93,    96,    44,    38,   124
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    50,    51,    52,    52,    52,    53,    53,    54,    54,
      54,    54,    54,    54,    54,    55,    55,    56,    56,    56,
      57,    57,    57,    57,    57,    57,    57,    57,    57,    57,
      57,    57,    57,    57,    57,    57,    57,    57,    57,    57,
      57,    57,    57,    57,    57,    57,    57,    58,    58,    59,
      59,    60,    60,    60,    61,    61,    61,    61,    61,    61,
      61,    62,    62,    63,    63,    64,    64,    65,    65,    65,
      65,    65,    65,    66,    66,    67,    67,    68,    68,    68,
      69,    69,    70,    70,    71,    71,    71,    71
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     1,     2,     1,     3,     5,     7,
       9,    11,     7,     1,     1,     2,     0,     1,     1,     3,
       1,     1,     3,     1,     4,     3,     1,     3,     3,     3,
       3,     3,     3,     2,     2,     2,     2,     2,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     1,     0,     3,
       1,     3,     5,     6,     1,     1,     1,     3,     1,     3,
       3,     1,     2,     1,     2,     5,     7,     1,     1,     1,
       3,     1,     3,     1,     2,     5,     7,     1,     1,     0,
       2,     0,     2,     0,     2,     2,     2,     0
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     0,     0,     0,    54,    55,    56,    58,     0,
      16,     0,     0,     2,     4,     6,    13,    61,    81,    14,
      63,     0,     0,     0,     0,     0,     0,    20,    21,    23,
       0,     0,     0,     0,    16,    48,     0,     0,    26,     0,
      67,    68,    69,    71,     0,    16,    73,    81,     0,     1,
       5,    58,     0,    62,    87,    64,     0,     0,     0,     0,
       3,    51,     0,     0,    37,    36,     0,    33,    35,    34,
       0,     0,    50,     0,    47,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    60,
      17,    18,     0,    57,    15,     0,     0,    74,    87,    59,
       0,     7,    80,     0,     0,     0,    83,    81,     0,     0,
       0,     0,     0,     0,     0,    45,    22,    25,     0,    44,
      39,    38,    43,    42,    41,    40,    28,    27,    31,    30,
      29,    32,    46,     0,    72,    70,    83,    81,    86,    85,
      84,     0,    79,    87,     3,     0,     3,     8,    52,     0,
      24,    49,    19,    79,    87,    82,    77,    78,    65,    83,
       0,     0,     0,    53,    75,    83,    79,     3,     9,     0,
      12,    79,    66,     0,     0,    76,    10,     3,     0,    11
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    12,    13,    14,    15,    39,    94,    37,    73,    74,
      38,    17,    18,    19,    20,    46,    47,    48,   158,    54,
     142,   106
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -150
static const yytype_int16 yypact[] =
{
      86,   -23,    -3,     1,   -10,  -150,  -150,  -150,   -11,   120,
    -150,   108,    45,  -150,    34,  -150,  -150,  -150,   138,  -150,
       9,   120,   120,   120,    37,   120,    10,  -150,  -150,    -5,
     120,    44,    75,   120,  -150,   120,   108,   222,  -150,    69,
    -150,  -150,  -150,  -150,   120,  -150,  -150,   158,   -36,  -150,
      86,  -150,   108,  -150,    21,  -150,   154,   239,   341,   256,
      86,   377,    74,   182,  -150,  -150,    10,    38,  -150,  -150,
     273,    73,   394,    49,    62,   -12,   120,   120,   120,   120,
     120,   120,   120,   120,   120,   120,   120,   120,    96,  -150,
    -150,  -150,   120,  -150,  -150,   290,   161,  -150,    21,  -150,
     108,  -150,  -150,   108,   108,   108,    94,   138,   115,   120,
     112,   113,   120,   103,   202,  -150,  -150,  -150,   120,  -150,
     377,   377,   377,   377,   377,   377,   153,   153,    38,    38,
      38,    38,  -150,   307,  -150,  -150,    94,   158,  -150,  -150,
    -150,   108,    64,    21,    86,   359,    86,  -150,   377,   120,
     103,   394,  -150,    64,    21,  -150,  -150,  -150,  -150,    94,
     101,   120,   121,   377,  -150,    94,    64,    86,  -150,   324,
    -150,    64,  -150,   125,   130,  -150,  -150,    86,   124,  -150
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -150,  -150,   -59,  -150,    71,    -9,  -150,   -14,  -150,  -150,
       0,   -16,    83,  -150,  -150,   -24,    40,   105,  -149,   -32,
    -130,   -95
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_int16 yytable[] =
{
      16,   111,    53,   136,   164,    24,   153,    57,    58,    59,
      99,    61,    63,   100,    25,    98,    67,   172,    21,    70,
      25,    72,   175,    97,    27,    71,    28,    29,   102,   166,
      95,    64,    65,    26,   119,   171,    96,   100,    22,    66,
      30,   103,    23,   104,   105,    49,    31,    32,   159,    60,
      16,    33,   114,    34,    35,    62,    36,    55,    56,   165,
      16,    68,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,    50,   143,    87,    88,   133,   138,
     139,   140,   156,   157,    90,   160,    91,   162,    90,     1,
      91,    53,    69,     2,   117,   145,     3,     4,   148,   112,
       5,     6,     7,     8,   151,   154,   167,   168,   173,   118,
      92,   132,    93,    97,    92,   141,   116,   155,   178,   144,
     146,   101,    40,    41,    42,    43,   147,     9,   149,    10,
     170,   176,    11,   179,    27,   163,    28,    29,   177,   107,
     137,    75,     0,     0,    16,     0,    16,   169,     0,    44,
      30,    45,     5,     6,     7,    51,    31,    32,     0,     0,
       0,    33,    52,    34,    35,     0,    36,    16,     5,     6,
       7,    51,    40,    41,    42,    43,    90,    16,    91,     9,
       0,    10,    52,     0,    11,    84,    85,    86,     0,     0,
       0,    87,    88,     0,     0,     9,     0,    10,     0,    44,
      11,    45,    92,     0,   135,    76,    77,     0,    78,    79,
      80,    81,    82,    83,    84,    85,    86,     0,     0,     0,
      87,    88,     0,     0,     0,    76,    77,   113,    78,    79,
      80,    81,    82,    83,    84,    85,    86,     0,     0,     0,
      87,    88,     0,     0,     0,    76,    77,   150,    78,    79,
      80,    81,    82,    83,    84,    85,    86,     0,     0,     0,
      87,    88,    76,    77,    89,    78,    79,    80,    81,    82,
      83,    84,    85,    86,     0,     0,     0,    87,    88,    76,
      77,   108,    78,    79,    80,    81,    82,    83,    84,    85,
      86,     0,     0,     0,    87,    88,    76,    77,   110,    78,
      79,    80,    81,    82,    83,    84,    85,    86,     0,     0,
       0,    87,    88,    76,    77,   115,    78,    79,    80,    81,
      82,    83,    84,    85,    86,     0,     0,     0,    87,    88,
      76,    77,   134,    78,    79,    80,    81,    82,    83,    84,
      85,    86,     0,     0,     0,    87,    88,    76,    77,   152,
      78,    79,    80,    81,    82,    83,    84,    85,    86,     0,
       0,     0,    87,    88,    76,    77,   174,    78,    79,    80,
      81,    82,    83,    84,    85,    86,     0,     0,     0,    87,
      88,   109,    76,    77,     0,    78,    79,    80,    81,    82,
      83,    84,    85,    86,     0,     0,     0,    87,    88,   161,
      -1,    -1,     0,    -1,    -1,    -1,    -1,    82,    83,    84,
      85,    86,     0,     0,     0,    87,    88,    76,    77,     0,
      78,    79,    80,    81,    82,    83,    84,    85,    86,     0,
       0,     0,    87,    88
};

static const yytype_int16 yycheck[] =
{
       0,    60,    18,    98,   153,    15,   136,    21,    22,    23,
      46,    25,    26,    49,    25,    47,    30,   166,    41,    33,
      25,    35,   171,    47,    14,    34,    16,    17,    52,   159,
      44,    36,    37,    44,    46,   165,    45,    49,    41,    44,
      30,    20,    41,    22,    23,     0,    36,    37,   143,    12,
      50,    41,    66,    43,    44,    45,    46,    48,    49,   154,
      60,    17,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    40,   107,    38,    39,    92,   103,
     104,   105,    18,    19,    15,   144,    17,   146,    15,     3,
      17,   107,    17,     7,    45,   109,    10,    11,   112,    25,
      14,    15,    16,    17,   118,   137,     5,     6,   167,    47,
      41,    15,    43,   137,    41,    21,    43,   141,   177,     4,
       8,    50,    14,    15,    16,    17,    13,    41,    25,    43,
       9,     6,    46,     9,    14,   149,    16,    17,     8,    56,
     100,    36,    -1,    -1,   144,    -1,   146,   161,    -1,    41,
      30,    43,    14,    15,    16,    17,    36,    37,    -1,    -1,
      -1,    41,    24,    43,    44,    -1,    46,   167,    14,    15,
      16,    17,    14,    15,    16,    17,    15,   177,    17,    41,
      -1,    43,    24,    -1,    46,    32,    33,    34,    -1,    -1,
      -1,    38,    39,    -1,    -1,    41,    -1,    43,    -1,    41,
      46,    43,    41,    -1,    43,    23,    24,    -1,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    -1,    -1,
      38,    39,    -1,    -1,    -1,    23,    24,    45,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    -1,    -1,
      38,    39,    -1,    -1,    -1,    23,    24,    45,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    -1,    -1,    -1,
      38,    39,    23,    24,    42,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    -1,    -1,    -1,    38,    39,    23,
      24,    42,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    -1,    -1,    -1,    38,    39,    23,    24,    42,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,    -1,
      -1,    38,    39,    23,    24,    42,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    -1,    -1,    -1,    38,    39,
      23,    24,    42,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,    -1,    -1,    38,    39,    23,    24,    42,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
      -1,    -1,    38,    39,    23,    24,    42,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    -1,    -1,    -1,    38,
      39,    40,    23,    24,    -1,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    -1,    -1,    -1,    38,    39,    40,
      23,    24,    -1,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,    -1,    -1,    38,    39,    23,    24,    -1,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    -1,
      -1,    -1,    38,    39
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     7,    10,    11,    14,    15,    16,    17,    41,
      43,    46,    51,    52,    53,    54,    60,    61,    62,    63,
      64,    41,    41,    41,    15,    25,    44,    14,    16,    17,
      30,    36,    37,    41,    43,    44,    46,    57,    60,    55,
      14,    15,    16,    17,    41,    43,    65,    66,    67,     0,
      40,    17,    24,    61,    69,    48,    49,    57,    57,    57,
      12,    57,    45,    57,    36,    37,    44,    57,    17,    17,
      57,    55,    57,    58,    59,    67,    23,    24,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    38,    39,    42,
      15,    17,    41,    43,    56,    57,    55,    65,    69,    46,
      49,    54,    65,    20,    22,    23,    71,    62,    42,    40,
      42,    52,    25,    45,    57,    42,    43,    45,    47,    46,
      57,    57,    57,    57,    57,    57,    57,    57,    57,    57,
      57,    57,    15,    57,    42,    43,    71,    66,    65,    65,
      65,    21,    70,    69,     4,    57,     8,    13,    57,    25,
      45,    57,    42,    70,    69,    65,    18,    19,    68,    71,
      52,    40,    52,    57,    68,    71,    70,     5,     6,    57,
       9,    70,    68,    52,    42,    68,     6,     8,    52,     9
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
#line 76 "script.y"
	{ free((yyvaluep->strval)); };

/* Line 1000 of yacc.c  */
#line 1315 "parser.c"
	break;
      case 16: /* "T_STRING_SCONST" */

/* Line 1000 of yacc.c  */
#line 76 "script.y"
	{ free((yyvaluep->strval)); };

/* Line 1000 of yacc.c  */
#line 1324 "parser.c"
	break;
      case 17: /* "T_VAR" */

/* Line 1000 of yacc.c  */
#line 76 "script.y"
	{ free((yyvaluep->strval)); };

/* Line 1000 of yacc.c  */
#line 1333 "parser.c"
	break;
      case 52: /* "stmtlist" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1342 "parser.c"
	break;
      case 53: /* "stmtlistr" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1351 "parser.c"
	break;
      case 54: /* "stmt" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1360 "parser.c"
	break;
      case 55: /* "strlist" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1369 "parser.c"
	break;
      case 56: /* "strcomp" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1378 "parser.c"
	break;
      case 57: /* "expr" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1387 "parser.c"
	break;
      case 58: /* "exprlist" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1396 "parser.c"
	break;
      case 59: /* "neexprlist" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1405 "parser.c"
	break;
      case 60: /* "assignstmt" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1414 "parser.c"
	break;
      case 61: /* "cmdexpr" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1423 "parser.c"
	break;
      case 62: /* "cmdexprlist" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1432 "parser.c"
	break;
      case 63: /* "cmd" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1441 "parser.c"
	break;
      case 64: /* "subcmd" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1450 "parser.c"
	break;
      case 65: /* "nestedcmdexpr" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1459 "parser.c"
	break;
      case 66: /* "nestedcmdexprlist" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1468 "parser.c"
	break;
      case 67: /* "nestedsubcmd" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1477 "parser.c"
	break;
      case 68: /* "cmdredirfd" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1486 "parser.c"
	break;
      case 69: /* "cmdredirin" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1495 "parser.c"
	break;
      case 70: /* "cmdredirerr" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1504 "parser.c"
	break;
      case 71: /* "cmdredirout" */

/* Line 1000 of yacc.c  */
#line 77 "script.y"
	{ ast_destroy((yyvaluep->node)); };

/* Line 1000 of yacc.c  */
#line 1513 "parser.c"
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
#line 82 "script.y"
    {
				ast_execute(curEnv,(yyvsp[(1) - (1)].node));
				ast_destroy((yyvsp[(1) - (1)].node));
			;}
    break;

  case 3:

/* Line 1455 of yacc.c  */
#line 89 "script.y"
    { (yyval.node) = ast_createStmtList(); ;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 90 "script.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); ;}
    break;

  case 5:

/* Line 1455 of yacc.c  */
#line 91 "script.y"
    { (yyval.node) = (yyvsp[(1) - (2)].node); ;}
    break;

  case 6:

/* Line 1455 of yacc.c  */
#line 95 "script.y"
    { (yyval.node) = ast_createStmtList(); ast_addStmt((yyval.node),(yyvsp[(1) - (1)].node)); ;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 96 "script.y"
    { (yyval.node) = ast_addStmt((yyvsp[(1) - (3)].node),(yyvsp[(3) - (3)].node)); ;}
    break;

  case 8:

/* Line 1455 of yacc.c  */
#line 100 "script.y"
    {
				(yyval.node) = ast_createFunctionStmt((yyvsp[(2) - (5)].strval),(yyvsp[(4) - (5)].node));
			;}
    break;

  case 9:

/* Line 1455 of yacc.c  */
#line 103 "script.y"
    {
				(yyval.node) = ast_createIfStmt((yyvsp[(3) - (7)].node),(yyvsp[(6) - (7)].node),NULL);
			;}
    break;

  case 10:

/* Line 1455 of yacc.c  */
#line 106 "script.y"
    {
				(yyval.node) = ast_createIfStmt((yyvsp[(3) - (9)].node),(yyvsp[(6) - (9)].node),(yyvsp[(8) - (9)].node));
			;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 109 "script.y"
    {
				(yyval.node) = ast_createForStmt((yyvsp[(3) - (11)].node),(yyvsp[(5) - (11)].node),(yyvsp[(7) - (11)].node),(yyvsp[(10) - (11)].node));
			;}
    break;

  case 12:

/* Line 1455 of yacc.c  */
#line 112 "script.y"
    {
				(yyval.node) = ast_createWhileStmt((yyvsp[(3) - (7)].node),(yyvsp[(6) - (7)].node));
			;}
    break;

  case 13:

/* Line 1455 of yacc.c  */
#line 115 "script.y"
    {
				(yyval.node) = (yyvsp[(1) - (1)].node);
			;}
    break;

  case 14:

/* Line 1455 of yacc.c  */
#line 118 "script.y"
    {
				(yyval.node) = (yyvsp[(1) - (1)].node);
			;}
    break;

  case 15:

/* Line 1455 of yacc.c  */
#line 124 "script.y"
    { (yyval.node) = (yyvsp[(1) - (2)].node); ast_addDStrComp((yyvsp[(1) - (2)].node),(yyvsp[(2) - (2)].node)); ;}
    break;

  case 16:

/* Line 1455 of yacc.c  */
#line 125 "script.y"
    { (yyval.node) = ast_createDStrExpr(); ;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 129 "script.y"
    { (yyval.node) = ast_createConstStrExpr((yyvsp[(1) - (1)].strval),false); ;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 130 "script.y"
    { (yyval.node) = ast_createVarExpr((yyvsp[(1) - (1)].strval),NULL); ;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 131 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 135 "script.y"
    { (yyval.node) = ast_createIntExpr((yyvsp[(1) - (1)].intval)); ;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 136 "script.y"
    { (yyval.node) = ast_createConstStrExpr((yyvsp[(1) - (1)].strval),true); ;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 137 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 138 "script.y"
    { (yyval.node) = ast_createVarExpr((yyvsp[(1) - (1)].strval),NULL); ;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 139 "script.y"
    { (yyval.node) = ast_createVarExpr((yyvsp[(1) - (4)].strval),(yyvsp[(3) - (4)].node)); ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 140 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 141 "script.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); ;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 142 "script.y"
    { (yyval.node) = ast_createBinOpExpr((yyvsp[(1) - (3)].node),'+',(yyvsp[(3) - (3)].node)); ;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 143 "script.y"
    { (yyval.node) = ast_createBinOpExpr((yyvsp[(1) - (3)].node),'-',(yyvsp[(3) - (3)].node)); ;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 144 "script.y"
    { (yyval.node) = ast_createBinOpExpr((yyvsp[(1) - (3)].node),'*',(yyvsp[(3) - (3)].node)); ;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 145 "script.y"
    { (yyval.node) = ast_createBinOpExpr((yyvsp[(1) - (3)].node),'/',(yyvsp[(3) - (3)].node)); ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 146 "script.y"
    { (yyval.node) = ast_createBinOpExpr((yyvsp[(1) - (3)].node),'%',(yyvsp[(3) - (3)].node)); ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 147 "script.y"
    { (yyval.node) = ast_createBinOpExpr((yyvsp[(1) - (3)].node),'^',(yyvsp[(3) - (3)].node)); ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 148 "script.y"
    { (yyval.node) = ast_createUnaryOpExpr((yyvsp[(2) - (2)].node),UN_OP_NEG); ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 149 "script.y"
    { (yyval.node) = ast_createUnaryOpExpr(ast_createVarExpr((yyvsp[(2) - (2)].strval),NULL),UN_OP_PREINC); ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 150 "script.y"
    { (yyval.node) = ast_createUnaryOpExpr(ast_createVarExpr((yyvsp[(2) - (2)].strval),NULL),UN_OP_PREDEC); ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 151 "script.y"
    { (yyval.node) = ast_createUnaryOpExpr(ast_createVarExpr((yyvsp[(1) - (2)].strval),NULL),UN_OP_POSTINC); ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 152 "script.y"
    { (yyval.node) = ast_createUnaryOpExpr(ast_createVarExpr((yyvsp[(1) - (2)].strval),NULL),UN_OP_POSTDEC); ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 153 "script.y"
    { (yyval.node) = ast_createCmpExpr((yyvsp[(1) - (3)].node),CMP_OP_LT,(yyvsp[(3) - (3)].node)); ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 154 "script.y"
    { (yyval.node) = ast_createCmpExpr((yyvsp[(1) - (3)].node),CMP_OP_GT,(yyvsp[(3) - (3)].node)); ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 155 "script.y"
    { (yyval.node) = ast_createCmpExpr((yyvsp[(1) - (3)].node),CMP_OP_LEQ,(yyvsp[(3) - (3)].node)); ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 156 "script.y"
    { (yyval.node) = ast_createCmpExpr((yyvsp[(1) - (3)].node),CMP_OP_GEQ,(yyvsp[(3) - (3)].node)); ;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 157 "script.y"
    { (yyval.node) = ast_createCmpExpr((yyvsp[(1) - (3)].node),CMP_OP_EQ,(yyvsp[(3) - (3)].node)); ;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 158 "script.y"
    { (yyval.node) = ast_createCmpExpr((yyvsp[(1) - (3)].node),CMP_OP_NEQ,(yyvsp[(3) - (3)].node)); ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 159 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ast_setRetOutput((yyvsp[(2) - (3)].node),true); ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 160 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 161 "script.y"
    { (yyval.node) = ast_createProperty((yyvsp[(1) - (3)].node),(yyvsp[(3) - (3)].strval)); ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 166 "script.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 167 "script.y"
    { (yyval.node) = ast_createExprList(); ;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 171 "script.y"
    { (yyval.node) = (yyvsp[(1) - (3)].node); ast_addExpr((yyvsp[(1) - (3)].node),(yyvsp[(3) - (3)].node)); ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 172 "script.y"
    { (yyval.node) = ast_createExprList(); ast_addExpr((yyval.node),(yyvsp[(1) - (1)].node)); ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 176 "script.y"
    {
				(yyval.node) = ast_createAssignExpr(ast_createVarExpr((yyvsp[(1) - (3)].strval),NULL),(yyvsp[(3) - (3)].node),false,NULL);
			;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 179 "script.y"
    {
				(yyval.node) = ast_createAssignExpr(ast_createVarExpr((yyvsp[(1) - (5)].strval),NULL),(yyvsp[(5) - (5)].node),true,NULL); 
			;}
    break;

  case 53:

/* Line 1455 of yacc.c  */
#line 182 "script.y"
    {
				(yyval.node) = ast_createAssignExpr(ast_createVarExpr((yyvsp[(1) - (6)].strval),NULL),(yyvsp[(6) - (6)].node),true,(yyvsp[(3) - (6)].node));
			;}
    break;

  case 54:

/* Line 1455 of yacc.c  */
#line 188 "script.y"
    { (yyval.node) = ast_createIntExpr((yyvsp[(1) - (1)].intval)); ;}
    break;

  case 55:

/* Line 1455 of yacc.c  */
#line 189 "script.y"
    { (yyval.node) = ast_createConstStrExpr((yyvsp[(1) - (1)].strval),false); ;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 190 "script.y"
    { (yyval.node) = ast_createConstStrExpr((yyvsp[(1) - (1)].strval),true); ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 191 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 192 "script.y"
    { (yyval.node) = ast_createVarExpr((yyvsp[(1) - (1)].strval),NULL); ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 193 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ast_setRetOutput((yyvsp[(2) - (3)].node),true); ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 194 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 198 "script.y"
    { (yyval.node) = ast_createCmdExprList(); ast_addCmdExpr((yyval.node),(yyvsp[(1) - (1)].node)); ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 199 "script.y"
    { (yyval.node) = (yyvsp[(1) - (2)].node); ast_addCmdExpr((yyvsp[(1) - (2)].node),(yyvsp[(2) - (2)].node)); ;}
    break;

  case 63:

/* Line 1455 of yacc.c  */
#line 203 "script.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); ;}
    break;

  case 64:

/* Line 1455 of yacc.c  */
#line 204 "script.y"
    { (yyval.node) = (yyvsp[(1) - (2)].node); ast_setRunInBG((yyvsp[(1) - (2)].node),true); ;}
    break;

  case 65:

/* Line 1455 of yacc.c  */
#line 208 "script.y"
    {
				(yyval.node) = ast_createCommand();
				ast_addSubCmd((yyval.node),ast_createSubCmd((yyvsp[(1) - (5)].node),(yyvsp[(5) - (5)].node),(yyvsp[(2) - (5)].node),(yyvsp[(3) - (5)].node),(yyvsp[(4) - (5)].node)));
			;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 212 "script.y"
    {
				(yyval.node) = (yyvsp[(1) - (7)].node);
				ast_addSubCmd((yyvsp[(1) - (7)].node),ast_createSubCmd((yyvsp[(3) - (7)].node),(yyvsp[(7) - (7)].node),(yyvsp[(4) - (7)].node),(yyvsp[(5) - (7)].node),(yyvsp[(6) - (7)].node)));
			;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 222 "script.y"
    { (yyval.node) = ast_createIntExpr((yyvsp[(1) - (1)].intval)); ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 223 "script.y"
    { (yyval.node) = ast_createConstStrExpr((yyvsp[(1) - (1)].strval),false); ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 224 "script.y"
    { (yyval.node) = ast_createConstStrExpr((yyvsp[(1) - (1)].strval),true); ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 225 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 226 "script.y"
    { (yyval.node) = ast_createVarExpr((yyvsp[(1) - (1)].strval),NULL); ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 227 "script.y"
    { (yyval.node) = (yyvsp[(2) - (3)].node); ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 231 "script.y"
    { (yyval.node) = ast_createCmdExprList(); ast_addCmdExpr((yyval.node),(yyvsp[(1) - (1)].node)); ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 232 "script.y"
    { (yyval.node) = (yyvsp[(1) - (2)].node); ast_addCmdExpr((yyvsp[(1) - (2)].node),(yyvsp[(2) - (2)].node)); ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 236 "script.y"
    {
				(yyval.node) = ast_createCommand();
				ast_addSubCmd((yyval.node),ast_createSubCmd((yyvsp[(1) - (5)].node),(yyvsp[(5) - (5)].node),(yyvsp[(2) - (5)].node),(yyvsp[(3) - (5)].node),(yyvsp[(4) - (5)].node)));
			;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 240 "script.y"
    {
				(yyval.node) = (yyvsp[(1) - (7)].node);
				ast_addSubCmd((yyvsp[(1) - (7)].node),ast_createSubCmd((yyvsp[(3) - (7)].node),(yyvsp[(7) - (7)].node),(yyvsp[(4) - (7)].node),(yyvsp[(5) - (7)].node),(yyvsp[(6) - (7)].node)));
			;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 249 "script.y"
    { (yyval.node) = ast_createRedirFd(REDIR_ERR2OUT); ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 250 "script.y"
    { (yyval.node) = ast_createRedirFd(REDIR_OUT2ERR); ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 251 "script.y"
    { (yyval.node) = ast_createRedirFd(REDIR_NO); ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 255 "script.y"
    { (yyval.node) = ast_createRedirFile((yyvsp[(2) - (2)].node),REDIR_INFILE); ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 256 "script.y"
    { (yyval.node) = ast_createRedirFile(NULL,REDIR_OUTCREATE); ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 260 "script.y"
    { (yyval.node) = ast_createRedirFile((yyvsp[(2) - (2)].node),REDIR_OUTCREATE); ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 261 "script.y"
    { (yyval.node) = ast_createRedirFile(NULL,REDIR_OUTCREATE); ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 265 "script.y"
    { (yyval.node) = ast_createRedirFile((yyvsp[(2) - (2)].node),REDIR_OUTCREATE); ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 266 "script.y"
    { (yyval.node) = ast_createRedirFile((yyvsp[(2) - (2)].node),REDIR_OUTCREATE); ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 267 "script.y"
    { (yyval.node) = ast_createRedirFile((yyvsp[(2) - (2)].node),REDIR_OUTAPPEND); ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 268 "script.y"
    { (yyval.node) = ast_createRedirFile(NULL,REDIR_OUTCREATE); ;}
    break;



/* Line 1455 of yacc.c  */
#line 2455 "parser.c"
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
#line 271 "script.y"


