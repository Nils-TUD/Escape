/* required for us! */
%option noyywrap
%option stack
%option yylineno

%{
	#include <string.h>
	#include <stdio.h>
	#include <shell/shell.h>
	#include "parser.h"
	#include "lang.h"
%}

%{
	extern YYLTYPE yylloc;
	YYLTYPE yylastloc;
	int yycolumn = 1;
	int openGraves = 0;

	#define YY_INPUT(buf,result,max_size) \
		{ \
			int c = curIsStream ? getc(curStream) : *curLine++; \
			result = (c == '\0' || c == EOF) ? YY_NULL : (buf[0] = c, 1); \
		}

	#define YY_USER_ACTION yylloc.filename = filename; \
		if(yylineno > 1 && yylloc.first_line != yylineno) { \
			memcpy(&yylastloc,&yylloc,sizeof(YYLTYPE)); \
			yycolumn = 1; \
			*yylloc.line = '\0'; \
		} \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng; \
    /* '\n' is always alone */ \
    if(*yytext != '\n') \
    	strcat(yylloc.line,yytext);
%}

%s COMMENT
%s DCONSTSTR
%s EXPR
%s ASSIGNEXPR

%%

 /* single line comment */
<INITIAL>"#" {
	yy_push_state(COMMENT);
}
<COMMENT>\n {
	yy_pop_state();
}
<COMMENT>. {
	/* eat up all chars */
}

 /* if, for, while */
<INITIAL>"if" {
	return T_IF;
}
<INITIAL>"then" {
	return T_THEN;
}
<INITIAL>"else" {
	return T_ELSE;
}
<INITIAL>"fi" {
	return T_FI;
}
<INITIAL>"for" {
	return T_FOR;
}
<INITIAL>"do" {
	return T_DO;
}
<INITIAL>"done" {
	return T_DONE;
}
<INITIAL>"while" {
	return T_WHILE;
}
<INITIAL>"function" {
	return T_FUNCTION;
}
<INITIAL>"begin" {
	return T_BEGIN;
}
<INITIAL>"end" {
	return T_END;
}

 /* "( ... )" puts an expression in a command or elsewhere. that means we can do something like
  * "echo (4 * 2^10)". it also means that we can use *,+,/,... in commands with no problems since
  * they have just special meanings inside the "(...)". this does also work for if/for/while etc.
  * since they require "(...)" for the "header" part. */
 /* Note that we always push/pop the EXPR-state because we don't know at which count we would
  * really have to do that (we can have nested stuff like: echo (`foo (1+2)` + 2);) */

<INITIAL,DCONSTSTR,EXPR,ASSIGNEXPR>"(" {
	yy_push_state(EXPR);
	return *yytext;
}
<EXPR,ASSIGNEXPR>")" {
	yy_pop_state();
	return *yytext;
}

 /* numbers, variables and single-quote strings */
<EXPR,ASSIGNEXPR>[0-9]+ {
	yylval.intval = atoi(yytext);
	return T_NUMBER;
}
<INITIAL,EXPR,ASSIGNEXPR,DCONSTSTR>\$[a-zA-Z_0-9]+ {
	yylval.strval = strdup(yytext);
	return T_VAR;
}
<INITIAL,EXPR,ASSIGNEXPR>'[^']*' {
	yylval.strval = strndup(yytext + 1,strlen(yytext) - 2);
	return T_STRING_SCONST;
}

 /* double-quote strings: we can use variables and expressions (in brackets) in them */
<INITIAL,EXPR,ASSIGNEXPR>\" {
	yy_push_state(DCONSTSTR);
	return *yytext;
}
<DCONSTSTR>\" {
	yy_pop_state();
	return *yytext;
}
<DCONSTSTR>[^\"\(\$]+ {
	yylval.strval = strdup(yytext);
	return T_STRING;
}

 /* io-redirection */
<INITIAL>">>" {
	return T_APPEND;
}
<INITIAL>"2>&1" {
	return T_ERR2OUT;
}
<INITIAL>"1>&2" {
	return T_OUT2ERR;
}
<INITIAL>"2>" {
	return T_ERR2FILE;
}
<INITIAL>"1>" {
	return T_OUT2FILE;
}

 /* array stuff */
<INITIAL,EXPR,ASSIGNEXPR,DCONSTSTR>"["|"]" {
	return *yytext;
}
<EXPR,ASSIGNEXPR>","|"~"|"." {
	return *yytext;
}
 /* for properties */
<EXPR,ASSIGNEXPR>[a-zA-Z0-9]+ {
	yylval.strval = strdup(yytext);
	return T_STRING;
}


 /* comparation */
<EXPR,ASSIGNEXPR>"<=" {
	return T_LEQ;
}
<EXPR,ASSIGNEXPR>">=" {
	return T_GEQ;
}
<EXPR,ASSIGNEXPR>"==" {
	return T_EQ;
}
<EXPR,ASSIGNEXPR>"!=" {
	return T_NEQ;
}

 /* io-redirection / comparation */
<INITIAL,EXPR,ASSIGNEXPR>">"|"<" {
	return *yytext;
}

 /* arithmetic operators */
<EXPR,ASSIGNEXPR>"+" {
	return T_ADD;
}
<EXPR,ASSIGNEXPR>"-" {
	return T_SUB;
}
<EXPR,ASSIGNEXPR>"*" {
	return T_MUL;
}
<EXPR,ASSIGNEXPR>"/" {
	return T_DIV;
}
<EXPR,ASSIGNEXPR>"%" {
	return T_MOD;
}
<EXPR,ASSIGNEXPR>"^" {
	return T_POW;
}
<EXPR,ASSIGNEXPR>"++" {
	return T_INC;
}
<EXPR,ASSIGNEXPR>"--" {
	return T_DEC;
}

 /* the "`...`" allow us to execute a command and collect the output, so that we can do something
  * like $a := `wc -l file.txt`; */
 /* this is possible directly in commands and in expressions. the stuff inside `...` is of course
  * also a command, therefore we go into the INITIAL state again. to know whether its an opening
  * or closing ` we simply count them. since we don't allow nested `...` its always an closing `
  * when we're in the INITIAL-state and openGraves is > 0. */
<INITIAL>"`" {
	if(openGraves > 0) {
		yy_pop_state();
		openGraves--;
	}
	else {
		yy_push_state(INITIAL);
		openGraves++;
	}
	return *yytext;
}
 /* in an expression its always an opening ` */
<EXPR,ASSIGNEXPR>"`" {
	yy_push_state(INITIAL);
	openGraves++;
	return *yytext;
}

<INITIAL,EXPR>";" {
	return *yytext;
}
<INITIAL>"&"|"|" {
	return *yytext;
}

 /* assignment. we want to allow assignments directly in "command-level" and without "(...)".
  * therefore we start an ASSIGNEXPR (which is the same as EXPR except that we handle the ending
  * differently). the end of it is a \n, ; or EOF. */
<INITIAL>":=" {
	yy_push_state(ASSIGNEXPR);
	return T_ASSIGN;
}
<EXPR,ASSIGNEXPR>":=" {
	return T_ASSIGN;
}
<ASSIGNEXPR><<EOF>> {
	yy_pop_state();
	yyterminate();
}
<ASSIGNEXPR>"\n"|";" {
	yy_pop_state();
	return *yytext;
}

 /* whitespace */
<INITIAL,EXPR,ASSIGNEXPR>[ \t]+ {
	/* eat up whitespace */
}
<INITIAL,EXPR>"\n" {
	/* eat up whitespace */
}

 /* strings without quotes */
<INITIAL>[a-zA-Z0-9\x15\x80-\xFF_\-!?:~,\.\+\-\*/%\^=@]+ {
	yylval.strval = strdup(yytext);
	return T_STRING;
}

 /* everything else is ignored */
<INITIAL,EXPR,ASSIGNEXPR>. {
	yyerror("Unrecognized character %c",*yytext);
}
