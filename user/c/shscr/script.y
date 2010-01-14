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
	#include "ast/command.h"
	#include "ast/cmdexprlist.h"
	#include "ast/subcmd.h"
	#include "ast/redirfd.h"
	#include "ast/redirfile.h"
	#include "exec/env.h"
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

%type <node> stmtlist stmt expr cmdexpr cmdexprlist cmd subcmd cmdredirfd cmdredirin cmdredirout

%nonassoc '>' '<' T_LEQ T_GEQ T_EQ T_NEQ
%left '+' '-'
%left '*' '/' '%'
%left T_NEG
%right '^'

%%

start:
			stmtlist						{ ast_printTree($1,0); sEnv *e = env_create(); ast_execute(e,$1); env_print(e); }
;

stmtlist:
			/* empty */					{ $$ = ast_createStmtList(); }
			| stmtlist stmt			{ $$ = ast_addStmt($1,$2); }
;

stmt:
			T_VAR '=' expr ';'	{ $$ = ast_createAssignStmt(ast_createVarExpr($1),$3); }
			| T_IF '(' expr ')' T_THEN stmtlist T_FI {
				$$ = ast_createIfStmt($3,$6,NULL);
			}
			| T_IF '(' expr ')' T_THEN stmtlist T_ELSE stmtlist T_FI {
				$$ = ast_createIfStmt($3,$6,$8);
			}
			| cmd ';' {
				$$ = $1;
			}
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
			| '`' cmd '`'				{ $$ = $2; }
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
			cmdexpr							{ $$ = ast_createCmdExprList(); ast_addCmdExpr($$,$1); }
			| cmdexprlist cmdexpr { $$ = $1; ast_addCmdExpr($1,$2); }
;

cmd:
			subcmd							{ $$ = $1; }
			| subcmd '&'				{ $$ = $1; ast_setRunInBG($1,true); }
;

subcmd:
			cmdexprlist cmdredirfd cmdredirin cmdredirout {
				$$ = ast_createCommand();
				ast_addSubCmd($$,ast_createSubCmd($1,$2,$3,$4));
			}
			| subcmd '|' cmdexprlist cmdredirfd cmdredirin cmdredirout {
				$$ = $1;
				ast_addSubCmd($1,ast_createSubCmd($3,$4,$5,$6));
			}
;

cmdredirfd:
			T_ERR2OUT						{ $$ = ast_createRedirFd(REDIR_ERR2OUT); }
			| T_OUT2ERR					{ $$ = ast_createRedirFd(REDIR_OUT2ERR); }
			| /* empty */				{ $$ = ast_createRedirFd(REDIR_NO); }
;

cmdredirin:
			'<' cmdexpr					{ $$ = ast_createRedirFile($2,REDIR_INFILE); }
			| /* empty */				{ $$ = ast_createRedirFile(NULL,REDIR_OUTCREATE); }
;

cmdredirout:
			'>' cmdexpr					{ $$ = ast_createRedirFile($2,REDIR_OUTCREATE); }
			| T_APPEND cmdexpr	{ $$ = ast_createRedirFile($2,REDIR_OUTAPPEND); }
			| /* empty */				{ $$ = ast_createRedirFile(NULL,REDIR_OUTCREATE); }
;

%%
