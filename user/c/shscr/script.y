%code top {
	#include <stdio.h>
	int yylex (void);
	void yyerror (char const *);
	int bla;
}

%code requires {
	#include "ast/node.h"
	#include "ast/assignstmt.h"
	#include "ast/binaryopexpr.h"
	#include "ast/cmpexpr.h"
	#include "ast/conststrexpr.h"
	#include "ast/dynstrexpr.h"
	#include "ast/ifstmt.h"
	#include "ast/intexpr.h"
	#include "ast/stmtlist.h"
	#include "ast/unaryopexpr.h"
	#include "ast/varexpr.h"
}
%union {
	int intval;
	char *strval;
	sASTNode *node;
}

%token T_IF
%token T_THEN
%token T_ELSE
%token T_FI

%token <intval> T_NUMBER
%token <strval> T_STRING
%token <strval> T_STRING_SCONST
%token <strval> T_STRING_DCONST
%token <strval> T_VAR

%token T_ERR2OUT
%token T_OUT2ERR
%token T_APPEND

%type <node> stmtlist stmt expr cmdexpr

%nonassoc '>' '<' T_LEQ T_GEQ T_EQ T_NEQ
%left '+' '-'
%left '*' '/' '%'
%left T_NEG
%right '^'

%%

start:
			stmtlist						{ ast_printTree($1,0); }
;

stmtlist:
			/* empty */					{ $$ = ast_createStmtList(); }
			| stmtlist stmt			{ $$ = ast_addStmt($1,$2); }
;

stmt:
			T_VAR '=' expr			{ $$ = ast_createAssignStmt(ast_createVarExpr($1),$3); }
			| T_IF '(' expr ')' T_THEN stmtlist T_FI {
				$$ = ast_createIfStmt($3,$6,NULL);
			}
			| T_IF '(' expr ')' T_THEN stmtlist T_ELSE stmtlist T_FI {
				$$ = ast_createIfStmt($3,$6,$8);
			}
			| cmd
;

expr:
			T_NUMBER						{ $$ = ast_createIntExpr($1); }
			| T_STRING					{ $$ = ast_createConstStrExpr($1); }
			| T_STRING_SCONST		{ $$ = ast_createConstStrExpr($1); }
			| T_STRING_DCONST		{ $$ = ast_createConstStrExpr($1); }
			| T_VAR							{ $$ = ast_createVarExpr($1); }
			| expr '+' expr			{ $$ = ast_createBinOpExpr($1,'+',$3); }
			| expr '-' expr			{ $$ = ast_createBinOpExpr($1,'-',$3); }
			| expr '*' expr			{ $$ = ast_createBinOpExpr($1,'*',$3); }
			| expr '/' expr			{ $$ = ast_createBinOpExpr($1,'/',$3); }
			| expr '%' expr			{ $$ = ast_createBinOpExpr($1,'%',$3); }
			| expr '^' expr			{ $$ = ast_createBinOpExpr($1,'^',$3); }
			| '-' expr %prec T_NEG { $$ = ast_createUnaryOpExpr($2,UN_OP_NEG); }
			| expr '<' expr			{ $$ = ast_createCmpExpr($1,CMP_OP_LT,$3); }
			| expr '>' expr			{ $$ = ast_createCmpExpr($1,CMP_OP_GT,$3); }
			| expr T_LEQ expr		{ $$ = ast_createCmpExpr($1,CMP_OP_LEQ,$3); }
			| expr T_GEQ expr		{ $$ = ast_createCmpExpr($1,CMP_OP_GEQ,$3); }
			| expr T_EQ expr		{ $$ = ast_createCmpExpr($1,CMP_OP_EQ,$3); }
			| expr T_NEQ expr		{ $$ = ast_createCmpExpr($1,CMP_OP_NEQ,$3); }
			| '(' expr ')'			{ $$ = $2; }
			| '`' cmd '`'
;

cmdexpr:
			T_NUMBER						{ $$ = ast_createIntExpr($1); }
			| T_STRING					{ $$ = ast_createConstStrExpr($1); }
			| T_STRING_SCONST		{ $$ = ast_createConstStrExpr($1); }
			| T_STRING_DCONST		{ $$ = ast_createConstStrExpr($1); }
			| T_VAR							{ $$ = ast_createConstStrExpr($1); }
			| '{' expr '}'			{ $$ = $2; }
;

cmdexprlist:
			cmdexpr
			| cmdexprlist cmdexpr
;

cmd:
			subcmd
			| subcmd '&'
;

subcmd:
			cmdexprlist cmdredirfd cmdredirfile
			| subcmd '|' subcmd
			| subcmd ';' subcmd
;

cmdredirfd:
			T_ERR2OUT
			| T_OUT2ERR
			| /* empty */
;

cmdredirfile:
			'>' cmdexpr
			| T_APPEND cmdexpr
			| /* empty */
;

%%
