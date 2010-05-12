/* required for us! */
%option noyywrap

%{
	#include <string.h>
	#include <stdio.h>
	#include "parser.h"
	#include "lang.h"
	#include "shell.h"
	
	int openBrk = 0;
%}

%{
	#define YY_INPUT(buf,result,max_size) \
		{ \
			int c = curIsStream ? getc(curStream) : *curLine++; \
			result = (c == '\0' || c == EOF) ? YY_NULL : (buf[0] = c, 1); \
		}
%}

%s COMMENT
%s CMDEXPR
%s DCONSTSTR
%s EXPR

%%

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

<INITIAL,EXPR>"(" {
	lang_beginToken(yytext);
	if(openBrk++ == 0)
		yy_push_state(EXPR);
}
<EXPR>")" {
	lang_beginToken(yytext);
	if(--openBrk == 0)
		yy_pop_state();
}

<INITIAL,CMDEXPR>[0-9]+ {
	lang_beginToken(yytext);
	yylval.intval = atoi(yytext);
	return T_NUMBER;
}

<INITIAL,CMDEXPR,DCONSTSTR>\$[a-zA-Z_0-9]+ {
	lang_beginToken(yytext);
	yylval.strval = strdup(yytext + 1);
	return T_VAR;
}

<INITIAL,CMDEXPR>'[^']*' {
	lang_beginToken(yytext);
	yylval.strval = strndup(yytext + 1,strlen(yytext) - 2);
	return T_STRING_SCONST;
}

<INITIAL>\" {
	lang_beginToken(yytext);
	BEGIN(DCONSTSTR);
	return *yytext;
}

<DCONSTSTR>\" {
	lang_beginToken(yytext);
	BEGIN(INITIAL);
	return *yytext;
}

<DCONSTSTR>"{" {
	lang_beginToken(yytext);
	BEGIN(CMDEXPR);
	return *yytext;
}

<DCONSTSTR>[^\"\{\$]+ {
	lang_beginToken(yytext);
	yylval.strval = strdup(yytext);
	return T_STRING;
}

<CMDEXPR>"}" {
	lang_beginToken(yytext);
	BEGIN(DCONSTSTR);
	return *yytext;
}

<INITIAL,CMDEXPR>"<=" {
	lang_beginToken(yytext);
	return T_LEQ;
}
<INITIAL,CMDEXPR>">=" {
	lang_beginToken(yytext);
	return T_GEQ;
}
<INITIAL,CMDEXPR>"==" {
	lang_beginToken(yytext);
	return T_EQ;
}
<INITIAL,CMDEXPR>"!=" {
	lang_beginToken(yytext);
	return T_NEQ;
}
<INITIAL,CMDEXPR>">>" {
	lang_beginToken(yytext);
	return T_APPEND;
}
<INITIAL,CMDEXPR>"2>&1" {
	lang_beginToken(yytext);
	return T_ERR2OUT;
}
<INITIAL,CMDEXPR>"1>&2" {
	lang_beginToken(yytext);
	return T_OUT2ERR;
}

<INITIAL,CMDEXPR>">"|"<" {
	lang_beginToken(yytext);
	return *yytext;
}
<INITIAL,CMDEXPR>":=" {
	lang_beginToken(yytext);
	return T_ASSIGN;
}
<INITIAL,CMDEXPR>"\\+" {
	lang_beginToken(yytext);
	return T_ADD;
}
<INITIAL,CMDEXPR>"\\-" {
	lang_beginToken(yytext);
	return T_SUB;
}
<INITIAL,CMDEXPR>"\\*" {
	lang_beginToken(yytext);
	return T_MUL;
}
<INITIAL,CMDEXPR>"\\/" {
	lang_beginToken(yytext);
	return T_DIV;
}
<INITIAL,CMDEXPR>"\\%" {
	lang_beginToken(yytext);
	return T_MOD;
}
<INITIAL,CMDEXPR>"\\^" {
	lang_beginToken(yytext);
	return T_POW;
}

<INITIAL,CMDEXPR>"`"|"&"|";"|"|" {
	lang_beginToken(yytext);
	return *yytext;
}
<INITIAL,CMDEXPR>"("|")" {
	lang_beginToken(yytext);
	return *yytext;
}
<INITIAL>"{"|"}" {
	lang_beginToken(yytext);
	return *yytext;
}

<INITIAL,CMDEXPR>[ \t]+ {
	/* eat up whitespace */
	lang_beginToken(yytext);
}
<INITIAL,CMDEXPR>"\n" {
	/* eat up whitespace */
	lang_beginToken(yytext);
}

<INITIAL,CMDEXPR>[a-zA-Z0-9\x15\x80-\xFF_\-!?:~,\.\+\-\*/%\^=]+ {
	lang_beginToken(yytext);
	yylval.strval = strdup(yytext);
	return T_STRING;
}

<INITIAL,CMDEXPR>. {
	lang_beginToken(yytext);
	yyerror("Unrecognized character %c",*yytext);
	BEGIN(INITIAL);
}
