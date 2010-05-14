%code top {
	#define YYDEBUG 1
	#include <stdio.h>
	int yylex (void);
	void yyerror(char const *,...);
}

%code requires {
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
%token T_FOR
%token T_DO
%token T_DONE
%token T_WHILE
%token T_FUNCTION
%token T_BEGIN
%token T_END

%token <intval> T_NUMBER
%token <strval> T_STRING
%token <strval> T_STRING_SCONST
%token <strval> T_VAR

%token T_ERR2OUT
%token T_OUT2ERR
%token T_APPEND
%token T_ERR2FILE
%token T_OUT2FILE

%type <node> stmtlist stmtlistr stmt expr assignstmt cmdexpr cmdexprlist cmd subcmd 
%type <node> cmdredirfd cmdredirin cmdredirout cmdredirerr strlist strcomp nestedcmdexpr
%type <node> nestedcmdexprlist nestedsubcmd neexprlist exprlist

%nonassoc '>' '<' T_LEQ T_GEQ T_EQ T_NEQ T_ASSIGN
%left T_ADD T_SUB
%left T_MUL T_DIV T_MOD
%left T_NEG
%right T_INC T_DEC
%right T_POW
%left '.'

%destructor { free($$); } <strval>
%destructor { ast_destroy($$); } <node>

%%

start:
			stmtlist {
				ast_execute(curEnv,$1);
				ast_destroy($1);
			}
;

stmtlist:
			/* empty */							{ $$ = ast_createStmtList(); }
			| stmtlistr							{ $$ = $1; }
			| stmtlistr ';'					{ $$ = $1; }
;

stmtlistr:
			stmt										{ $$ = ast_createStmtList(); ast_addStmt($$,$1); }
			| stmtlistr ';' stmt		{ $$ = ast_addStmt($1,$3); }
;

stmt:
			T_FUNCTION T_STRING T_BEGIN stmtlist T_END {
				$$ = ast_createFunctionStmt($2,$4);
			}
			| T_IF '(' expr ')' T_THEN stmtlist T_FI {
				$$ = ast_createIfStmt($3,$6,NULL);
			}
			| T_IF '(' expr ')' T_THEN stmtlist T_ELSE stmtlist T_FI {
				$$ = ast_createIfStmt($3,$6,$8);
			}
			| T_FOR '(' expr ';' expr ';' expr ')' T_DO stmtlist T_DONE {
				$$ = ast_createForStmt($3,$5,$7,$10);
			}
			| T_WHILE '(' expr ')' T_DO stmtlist T_DONE {
				$$ = ast_createWhileStmt($3,$6);
			}
			| assignstmt	{
				$$ = $1;
			}
			| cmd {
				$$ = $1;
			}
;

strlist:
			strlist strcomp			{ $$ = $1; ast_addDStrComp($1,$2); }
			| /* empty */				{ $$ = ast_createDStrExpr(); }
;

strcomp:
			T_STRING						{ $$ = ast_createConstStrExpr($1,false); }
			| T_VAR							{ $$ = ast_createVarExpr($1,NULL); }
			| '(' expr ')'			{ $$ = $2; }
;

expr:
			T_NUMBER						{ $$ = ast_createIntExpr($1); }
			| T_STRING_SCONST		{ $$ = ast_createConstStrExpr($1,true); }
			| '"' strlist '"'		{ $$ = $2; }
			| T_VAR							{ $$ = ast_createVarExpr($1,NULL); }
			| T_VAR '[' expr ']' { $$ = ast_createVarExpr($1,$3); }
			| '[' exprlist ']'	{ $$ = $2; }
			| assignstmt				{ $$ = $1; }
			| expr T_ADD expr		{ $$ = ast_createBinOpExpr($1,'+',$3); }
			| expr T_SUB expr		{ $$ = ast_createBinOpExpr($1,'-',$3); }
			| expr T_MUL expr		{ $$ = ast_createBinOpExpr($1,'*',$3); }
			| expr T_DIV expr		{ $$ = ast_createBinOpExpr($1,'/',$3); }
			| expr T_MOD expr		{ $$ = ast_createBinOpExpr($1,'%',$3); }
			| expr T_POW expr		{ $$ = ast_createBinOpExpr($1,'^',$3); }
			| T_SUB expr %prec T_NEG { $$ = ast_createUnaryOpExpr($2,UN_OP_NEG); }
			| T_INC T_VAR				{ $$ = ast_createUnaryOpExpr(ast_createVarExpr($2,NULL),UN_OP_PREINC); }
			| T_DEC T_VAR				{ $$ = ast_createUnaryOpExpr(ast_createVarExpr($2,NULL),UN_OP_PREDEC); }
			| T_VAR T_INC				{ $$ = ast_createUnaryOpExpr(ast_createVarExpr($1,NULL),UN_OP_POSTINC); }
			| T_VAR T_DEC				{ $$ = ast_createUnaryOpExpr(ast_createVarExpr($1,NULL),UN_OP_POSTDEC); }
			| expr '<' expr			{ $$ = ast_createCmpExpr($1,CMP_OP_LT,$3); }
			| expr '>' expr			{ $$ = ast_createCmpExpr($1,CMP_OP_GT,$3); }
			| expr T_LEQ expr		{ $$ = ast_createCmpExpr($1,CMP_OP_LEQ,$3); }
			| expr T_GEQ expr		{ $$ = ast_createCmpExpr($1,CMP_OP_GEQ,$3); }
			| expr T_EQ expr		{ $$ = ast_createCmpExpr($1,CMP_OP_EQ,$3); }
			| expr T_NEQ expr		{ $$ = ast_createCmpExpr($1,CMP_OP_NEQ,$3); }
			| '`' nestedsubcmd '`'		{ $$ = $2; ast_setRetOutput($2,true); }
			| '(' expr ')'			{ $$ = $2; }
			| expr '.' T_STRING	{ $$ = ast_createProperty($1,$3); }
;

/* for comma-separated expressions; may be empty */
exprlist:
			neexprlist					{ $$ = $1; }
			| /* empty */				{ $$ = ast_createExprList(); }
;

neexprlist:
			neexprlist ',' expr		{ $$ = $1; ast_addExpr($1,$3); }
			| expr								{ $$ = ast_createExprList(); ast_addExpr($$,$1); }
;

assignstmt:
			T_VAR T_ASSIGN expr	{
				$$ = ast_createAssignExpr(ast_createVarExpr($1,NULL),$3,false,NULL);
			}
			| T_VAR '[' ']' T_ASSIGN expr {
				$$ = ast_createAssignExpr(ast_createVarExpr($1,NULL),$5,true,NULL); 
			}
			| T_VAR '[' expr ']' T_ASSIGN expr {
				$$ = ast_createAssignExpr(ast_createVarExpr($1,NULL),$6,true,$3);
			}
;

cmdexpr:
			T_NUMBER						{ $$ = ast_createIntExpr($1); }
			| T_STRING					{ $$ = ast_createConstStrExpr($1,false); }
			| T_STRING_SCONST		{ $$ = ast_createConstStrExpr($1,true); }
			| '"' strlist '"'		{ $$ = $2; }
			| T_VAR							{ $$ = ast_createVarExpr($1,NULL); }
			| '`' nestedsubcmd '`'		{ $$ = $2; ast_setRetOutput($2,true); }
			| '(' expr ')'			{ $$ = $2; }
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
			cmdexprlist cmdredirin cmdredirout cmdredirerr cmdredirfd {
				$$ = ast_createCommand();
				ast_addSubCmd($$,ast_createSubCmd($1,$5,$2,$3,$4));
			}
			| subcmd '|' cmdexprlist cmdredirin cmdredirout cmdredirerr cmdredirfd {
				$$ = $1;
				ast_addSubCmd($1,ast_createSubCmd($3,$7,$4,$5,$6));
			}
;

/* unfortunatly we need another version of cmdexpr, cmdexprlist and subcmd. because we can't allow
 * nesting of `...`-expressions (the end of it wouldn't be clear). Therefore we duplicate the stuff
 * and drop the '`' nestedsubcmd '`' part in cmdexpr. */
nestedcmdexpr:
			T_NUMBER						{ $$ = ast_createIntExpr($1); }
			| T_STRING					{ $$ = ast_createConstStrExpr($1,false); }
			| T_STRING_SCONST		{ $$ = ast_createConstStrExpr($1,true); }
			| '"' strlist '"'		{ $$ = $2; }
			| T_VAR							{ $$ = ast_createVarExpr($1,NULL); }
			| '(' expr ')'			{ $$ = $2; }
;

nestedcmdexprlist:
			nestedcmdexpr				{ $$ = ast_createCmdExprList(); ast_addCmdExpr($$,$1); }
			| nestedcmdexprlist nestedcmdexpr { $$ = $1; ast_addCmdExpr($1,$2); }
;

nestedsubcmd:
			nestedcmdexprlist cmdredirin cmdredirout cmdredirerr cmdredirfd {
				$$ = ast_createCommand();
				ast_addSubCmd($$,ast_createSubCmd($1,$5,$2,$3,$4));
			}
			| nestedsubcmd '|' nestedcmdexprlist cmdredirin cmdredirout cmdredirerr cmdredirfd {
				$$ = $1;
				ast_addSubCmd($1,ast_createSubCmd($3,$7,$4,$5,$6));
			}
;

/* I think in this case its not really necessary to redirect to a filename generated by a command */
/* So we use always nestedcmdexpr */
cmdredirfd:
			T_ERR2OUT						{ $$ = ast_createRedirFd(REDIR_ERR2OUT); }
			| T_OUT2ERR					{ $$ = ast_createRedirFd(REDIR_OUT2ERR); }
			| /* empty */				{ $$ = ast_createRedirFd(REDIR_NO); }
;

cmdredirin:
			'<' nestedcmdexpr		{ $$ = ast_createRedirFile($2,REDIR_INFILE); }
			| /* empty */				{ $$ = ast_createRedirFile(NULL,REDIR_OUTCREATE); }
;

cmdredirerr:
			T_ERR2FILE nestedcmdexpr { $$ = ast_createRedirFile($2,REDIR_OUTCREATE); }
			| /* empty */				{ $$ = ast_createRedirFile(NULL,REDIR_OUTCREATE); }
;

cmdredirout:
			'>' nestedcmdexpr		{ $$ = ast_createRedirFile($2,REDIR_OUTCREATE); }
			| T_OUT2FILE nestedcmdexpr { $$ = ast_createRedirFile($2,REDIR_OUTCREATE); }
			| T_APPEND nestedcmdexpr	{ $$ = ast_createRedirFile($2,REDIR_OUTAPPEND); }
			| /* empty */				{ $$ = ast_createRedirFile(NULL,REDIR_OUTCREATE); }
;

%%
