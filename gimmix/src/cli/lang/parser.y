%locations
%code top {
	#include <stdio.h>
	#include <string.h>
	
	#include "common.h"
	#include "cli/lang/ast.h"
	#include "cli/console.h"
	#include "mmix/mem.h"
	
	#define YYERROR_VERBOSE	1 /* show unexpected and expected token */
	
	extern int yylex (void);
	extern void yyerror(char const *,...);
}

%code requires {
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
}

%union {
	octa intval;
	double floatval;
	char *strval;
	sASTNode *nodeval;
}

%token T_WS T_NL T_EOF
%token T_VMEM T_VMEM1 T_VMEM2 T_VMEM4 T_VMEM8 T_PMEM
%token T_LREG T_GREG T_SREG T_DREG T_AT
%token T_LBR T_RBR T_COLON T_DOTDOT
%token T_ADD T_MINUS T_MUL T_MULU T_DIV T_DIVU T_MOD T_MODU T_SR T_SAR T_AND T_OR T_XOR T_NOT

%token <floatval> T_FLOAT
%token <intval> T_INTEGER T_DREGLIT
%token <strval> T_IDENT

%type <nodeval> stmtlist nestmtlist stmt exprlist neexprlist expr

%nonassoc T_COLON T_DOTDOT
%left T_OR
%left T_XOR
%left T_AND
%left T_SL T_SR T_SAR
%left T_ADD T_SUB
%left T_MUL T_MULU T_DIV T_DIVU T_MOD T_MODU
%left T_NEG T_NOT
%nonassoc T_LBR T_RBR
%nonassoc T_LPAR T_RPAR

%destructor { mem_free($$); } <strval>
%destructor { ast_destroy($$); } <nodeval>

%%

start:
		stmtlist					{ cons_setAST($1); }
		/* allow newlines at the beginning */
		| T_NL stmtlist				{ cons_setAST($2); }
;

stmtlist:
		/* empty */					{ $$ = ast_createStmtList(NULL,NULL); }
		| nestmtlist				{ $$ = $1; }
		/* allow newlines at the end */
		| nestmtlist T_NL			{ $$ = $1; }
;

nestmtlist:
		stmt						{ $$ = ast_createStmtList($1,ast_createStmtList(NULL,NULL)); }
		| nestmtlist T_NL stmt		{ $$ = ast_createStmtList($3,$1); }
;

stmt:
		T_IDENT exprlist			{ $$ = ast_createCmdStmt($1,$2); }
		/* allow whitespace at the beginning */
		| T_WS T_IDENT exprlist		{ $$ = ast_createCmdStmt($2,$3); }
;

exprlist:
		/* empty */					{ $$ = ast_createExprList(NULL,NULL); }
		| T_WS neexprlist			{ $$ = $2; }
		/* allow whitespace at the end */
		| T_WS neexprlist T_WS		{ $$ = $2; }
;

neexprlist:
		expr						{ $$ = ast_createExprList($1,ast_createExprList(NULL,NULL)); }
		| neexprlist T_WS expr		{ $$ = ast_createExprList($3,$1); }
;

expr:
		T_IDENT						{ $$ = ast_createStrExpr($1); }
		| T_INTEGER					{ $$ = ast_createIntExpr($1); }
		| T_FLOAT					{ $$ = ast_createFloatExpr($1); }
		| T_AT						{ $$ = ast_createPCExpr(); }
		| expr T_ADD expr			{ $$ = ast_createBinOpExpr(BINOP_ADD,$1,$3); }
		| expr T_SUB expr			{ $$ = ast_createBinOpExpr(BINOP_SUB,$1,$3); }
		| expr T_MUL expr			{ $$ = ast_createBinOpExpr(BINOP_MUL,$1,$3); }
		| expr T_MULU expr			{ $$ = ast_createBinOpExpr(BINOP_MULU,$1,$3); }
		| expr T_DIV expr			{ $$ = ast_createBinOpExpr(BINOP_DIV,$1,$3); }
		| expr T_DIVU expr			{ $$ = ast_createBinOpExpr(BINOP_DIVU,$1,$3); }
		| expr T_MOD expr			{ $$ = ast_createBinOpExpr(BINOP_MOD,$1,$3); }
		| expr T_MODU expr			{ $$ = ast_createBinOpExpr(BINOP_MODU,$1,$3); }
		| expr T_AND expr			{ $$ = ast_createBinOpExpr(BINOP_AND,$1,$3); }
		| expr T_OR expr			{ $$ = ast_createBinOpExpr(BINOP_OR,$1,$3); }
		| expr T_XOR expr			{ $$ = ast_createBinOpExpr(BINOP_XOR,$1,$3); }
		| expr T_SL expr			{ $$ = ast_createBinOpExpr(BINOP_SL,$1,$3); }
		| expr T_SAR expr			{ $$ = ast_createBinOpExpr(BINOP_SAR,$1,$3); }
		| expr T_SR expr			{ $$ = ast_createBinOpExpr(BINOP_SR,$1,$3); }
		| T_NOT expr				{ $$ = ast_createUnOpExpr(UNOP_NOT,$2); }
		| T_SUB expr %prec T_NEG	{ $$ = ast_createUnOpExpr(UNOP_NEG,$2); }
		| expr T_DOTDOT expr		{ $$ = ast_createRangeExpr(RANGE_FROMTO,$1,$3); }
		| expr T_COLON expr			{ $$ = ast_createRangeExpr(RANGE_FROMCNT,$1,$3); }
		| T_VMEM T_LBR expr T_RBR 	{ $$ = ast_createFetchExpr(FETCH_VMEM,$3); }
		| T_VMEM1 T_LBR expr T_RBR 	{ $$ = ast_createFetchExpr(FETCH_VMEM1,$3); }
		| T_VMEM2 T_LBR expr T_RBR 	{ $$ = ast_createFetchExpr(FETCH_VMEM2,$3); }
		| T_VMEM4 T_LBR expr T_RBR 	{ $$ = ast_createFetchExpr(FETCH_VMEM4,$3); }
		| T_VMEM8 T_LBR expr T_RBR 	{ $$ = ast_createFetchExpr(FETCH_VMEM8,$3); }
		| T_PMEM T_LBR expr T_RBR 	{ $$ = ast_createFetchExpr(FETCH_PMEM,$3); }
		| T_LREG T_LBR expr T_RBR	{ $$ = ast_createFetchExpr(FETCH_LREG,$3); }
		| T_GREG T_LBR expr T_RBR	{ $$ = ast_createFetchExpr(FETCH_GREG,$3); }
		| T_SREG T_LBR expr T_RBR	{ $$ = ast_createFetchExpr(FETCH_SREG,$3); }
		| T_DREG T_LBR expr T_RBR 	{ $$ = ast_createFetchExpr(FETCH_DREG,$3); }
		| T_DREGLIT					{ $$ = ast_createFetchExpr(FETCH_DREG,ast_createIntExpr($1)); }
		| T_LPAR expr T_RPAR		{ $$ = $2; }
;
