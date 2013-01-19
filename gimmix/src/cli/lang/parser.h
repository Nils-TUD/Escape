/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
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

/* "%code requires" blocks.  */

/* Line 2068 of yacc.c  */
#line 17 "cli/lang/parser.y"

	typedef struct YYLTYPE {
	  int first_line;
	  int first_column;
	  int last_line;
	  int last_column;
	  const char *filename;
	  char line[MAX_LINE_LEN + 1];
	} YYLTYPE;
	
	#define YYLTYPE_IS_DECLARED 1 /* alert the parser that we have our own definition */
	
	#define YYLLOC_DEFAULT(Current, Rhs, N)                                \
	    do                                                                 \
	      if (N)                                                           \
	        {                                                              \
	          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;       \
	          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;     \
	          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;        \
	          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;      \
	          (Current).filename     = YYRHSLOC (Rhs, 1).filename;         \
	          strncpy((Current).line,YYRHSLOC(Rhs,1).line,MAX_LINE_LEN);   \
	          (Current).line[MAX_LINE_LEN] = '\0';                         \
	        }                                                              \
	      else                                                             \
	        { /* empty RHS */                                              \
	          (Current).first_line   = (Current).last_line   =             \
	            YYRHSLOC (Rhs, 0).last_line;                               \
	          (Current).first_column = (Current).last_column =             \
	            YYRHSLOC (Rhs, 0).last_column;                             \
	          (Current).filename  = NULL;                                  \
	          strncpy((Current).line,YYRHSLOC(Rhs,0).line,MAX_LINE_LEN);   \
	          (Current).line[MAX_LINE_LEN] = '\0';                         \
	        }                                                              \
	    while (0)



/* Line 2068 of yacc.c  */
#line 77 "cli/lang/parser.h"

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_WS = 258,
     T_NL = 259,
     T_EOF = 260,
     T_VMEM = 261,
     T_VMEM1 = 262,
     T_VMEM2 = 263,
     T_VMEM4 = 264,
     T_VMEM8 = 265,
     T_PMEM = 266,
     T_LREG = 267,
     T_GREG = 268,
     T_SREG = 269,
     T_DREG = 270,
     T_AT = 271,
     T_LBR = 272,
     T_RBR = 273,
     T_COLON = 274,
     T_DOTDOT = 275,
     T_ADD = 276,
     T_MINUS = 277,
     T_MUL = 278,
     T_MULU = 279,
     T_DIV = 280,
     T_DIVU = 281,
     T_MOD = 282,
     T_MODU = 283,
     T_SR = 284,
     T_SAR = 285,
     T_AND = 286,
     T_OR = 287,
     T_XOR = 288,
     T_NOT = 289,
     T_FLOAT = 290,
     T_INTEGER = 291,
     T_DREGLIT = 292,
     T_IDENT = 293,
     T_SL = 294,
     T_SUB = 295,
     T_NEG = 296,
     T_RPAR = 297,
     T_LPAR = 298
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 54 "cli/lang/parser.y"

	octa intval;
	double floatval;
	char *strval;
	sASTNode *nodeval;



/* Line 2068 of yacc.c  */
#line 146 "cli/lang/parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYLTYPE yylloc;

