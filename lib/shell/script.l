/* required for us! */
%option noyywrap
%option stack

%{
	#include <string.h>
	#include <stdio.h>
	#include "parser.h"
	#include "lang.h"
	#include "shell.h"
%}

%{
	#define YY_INPUT(buf,result,max_size) \
		{ \
			int c = curIsStream ? getc(curStream) : *curLine++; \
			result = (c == '\0' || c == EOF) ? YY_NULL : (buf[0] = c, 1); \
		}
%}

%s COMMENT
%s DCONSTSTR
%s EXPR
%s ASSIGNEXPR

%%

 /* single line comment */
<INITIAL>"#" {
	lang_beginToken(yytext);
	yy_push_state(COMMENT);
}
<COMMENT>\n {
	lang_beginToken(yytext);
	yy_pop_state();
}
<COMMENT>. {
	lang_beginToken(yytext);
	/* eat up all chars */
}

 /* if, for, while */
<INITIAL>"if" {
	lang_beginToken(yytext);
	return T_IF;
}
<INITIAL>"then" {
	lang_beginToken(yytext);
	return T_THEN;
}
<INITIAL>"else" {
	lang_beginToken(yytext);
	return T_ELSE;
}
<INITIAL>"fi" {
	lang_beginToken(yytext);
	return T_FI;
}
<INITIAL>"for" {
	lang_beginToken(yytext);
	return T_FOR;
}
<INITIAL>"do" {
	lang_beginToken(yytext);
	return T_DO;
}
<INITIAL>"done" {
	lang_beginToken(yytext);
	return T_DONE;
}
<INITIAL>"while" {
	lang_beginToken(yytext);
	return T_WHILE;
}

 /* "( ... )" puts an expression in a command or elsewhere. that means we can do something like
  * "echo (4 * 2^10)". it also means that we can use *,+,/,... in commands with no problems since
  * they have just special meanings inside the "(...)". this does also work for if/for/while etc.
  * since they require "(...)" for the "header" part. */
 /* therefore we simply count the brackets to know where we are */

<INITIAL,EXPR,ASSIGNEXPR,DCONSTSTR>"(" {
	lang_beginToken(yytext);
	if(openBrk++ == 0)
		yy_push_state(EXPR);
	return *yytext;
}
<EXPR,ASSIGNEXPR>")" {
	lang_beginToken(yytext);
	if(--openBrk == 0)
		yy_pop_state();
	return *yytext;
}

 /* numbers, variables and single-quote strings */
<EXPR,ASSIGNEXPR>[0-9]+ {
	lang_beginToken(yytext);
	yylval.intval = atoi(yytext);
	return T_NUMBER;
}
<INITIAL,EXPR,ASSIGNEXPR,DCONSTSTR>\$[a-zA-Z_0-9]+ {
	lang_beginToken(yytext);
	yylval.strval = strdup(yytext + 1);
	return T_VAR;
}
<INITIAL,EXPR,ASSIGNEXPR>'[^']*' {
	lang_beginToken(yytext);
	yylval.strval = strndup(yytext + 1,strlen(yytext) - 2);
	return T_STRING_SCONST;
}

 /* double-quote strings: we can use variables and expressions (in brackets) in them */
<INITIAL,EXPR,ASSIGNEXPR>\" {
	lang_beginToken(yytext);
	yy_push_state(DCONSTSTR);
	return *yytext;
}
<DCONSTSTR>\" {
	lang_beginToken(yytext);
	yy_pop_state();
	return *yytext;
}
<DCONSTSTR>[^\"\(\$]+ {
	lang_beginToken(yytext);
	yylval.strval = strdup(yytext);
	return T_STRING;
}

 /* io-redirection */
<INITIAL>">>" {
	lang_beginToken(yytext);
	return T_APPEND;
}
<INITIAL>"2>&1" {
	lang_beginToken(yytext);
	return T_ERR2OUT;
}
<INITIAL>"1>&2" {
	lang_beginToken(yytext);
	return T_OUT2ERR;
}

 /* comparation */
<EXPR,ASSIGNEXPR>"<=" {
	lang_beginToken(yytext);
	return T_LEQ;
}
<EXPR,ASSIGNEXPR>">=" {
	lang_beginToken(yytext);
	return T_GEQ;
}
<EXPR,ASSIGNEXPR>"==" {
	lang_beginToken(yytext);
	return T_EQ;
}
<EXPR,ASSIGNEXPR>"!=" {
	lang_beginToken(yytext);
	return T_NEQ;
}

 /* io-redirection / comparation */
<INITIAL,EXPR,ASSIGNEXPR>">"|"<" {
	lang_beginToken(yytext);
	return *yytext;
}

 /* arithmetic operators */
<EXPR,ASSIGNEXPR>"+" {
	lang_beginToken(yytext);
	return T_ADD;
}
<EXPR,ASSIGNEXPR>"-" {
	lang_beginToken(yytext);
	return T_SUB;
}
<EXPR,ASSIGNEXPR>"*" {
	lang_beginToken(yytext);
	return T_MUL;
}
<EXPR,ASSIGNEXPR>"/" {
	lang_beginToken(yytext);
	return T_DIV;
}
<EXPR,ASSIGNEXPR>"%" {
	lang_beginToken(yytext);
	return T_MOD;
}
<EXPR,ASSIGNEXPR>"^" {
	lang_beginToken(yytext);
	return T_POW;
}

 /* the "`...`" allow us to execute a command and collect the output, so that we can do something
  * like $a := `wc -l file.txt`; */
 /* this is possible directly in commands and in expressions. the stuff inside `...` is of course
  * also a command, therefore we go into the INITIAL state again. to know wether its an opening
  * or closing ` we simply count them. since we don't allow nested `...` its always an closing `
  * when we're in the INITIAL-state and openGraves is > 0. */
<INITIAL>"`" {
	lang_beginToken(yytext);
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
	lang_beginToken(yytext);
	yy_push_state(INITIAL);
	openGraves++;
	return *yytext;
}

<INITIAL,EXPR>";" {
	lang_beginToken(yytext);
	return *yytext;
}
<INITIAL>"&"|"|" {
	lang_beginToken(yytext);
	return *yytext;
}

 /* assignment. we want to allow assignments directly in "command-level" and without "(...)".
  * therefore we start an ASSIGNEXPR (which is the same as EXPR except that we handle the ending
  * differently). the end of it is a \n, ; or EOF. */
<INITIAL>":=" {
	lang_beginToken(yytext);
	yy_push_state(ASSIGNEXPR);
	return T_ASSIGN;
}
<EXPR,ASSIGNEXPR>":=" {
	lang_beginToken(yytext);
	return T_ASSIGN;
}
<ASSIGNEXPR><<EOF>> {
	lang_beginToken(yytext);
	yy_pop_state();
	yyterminate();
}
<ASSIGNEXPR>"\n"|";" {
	lang_beginToken(yytext);
	yy_pop_state();
	return *yytext;
}

 /* whitespace */
<INITIAL,EXPR,ASSIGNEXPR>[ \t]+ {
	/* eat up whitespace */
	lang_beginToken(yytext);
}
<INITIAL,EXPR>"\n" {
	/* eat up whitespace */
	lang_beginToken(yytext);
}

 /* strings without quotes */
<INITIAL>[a-zA-Z0-9\x15\x80-\xFF_\-!?:~,\.\+\-\*/%\^=]+ {
	lang_beginToken(yytext);
	yylval.strval = strdup(yytext);
	return T_STRING;
}

 /* everything else is ignored */
<INITIAL,EXPR,ASSIGNEXPR>. {
	lang_beginToken(yytext);
	yyerror("Unrecognized character %c",*yytext);
}
