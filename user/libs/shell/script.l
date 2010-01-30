/* required for us! */
%option noyywrap

%{
	#include <string.h>
	#include "parser.h"
	#include "lang.h"
	#include "shell.h"
%}

%{
	#define YY_INPUT(buf,result,max_size) \
		{ \
			int c = curIsStream ? fscanc(curStream) : *curLine++; \
			result = (c == '\0' || c == EOF) ? YY_NULL : (buf[0] = c, 1); \
		}
%}

%s COMMENT
%s CMDEXPR
%s DCONSTSTR

%%

<INITIAL>"#" {
	beginToken(yytext);
	BEGIN(COMMENT);
}
<COMMENT>\n {
	beginToken(yytext);
	BEGIN(INITIAL);
}
<COMMENT>. {
	beginToken(yytext);
	/* eat up all chars */
}

<INITIAL>"if" {
	beginToken(yytext);
	return T_IF;
}
<INITIAL>"then" {
	beginToken(yytext);
	return T_THEN;
}
<INITIAL>"else" {
	beginToken(yytext);
	return T_ELSE;
}
<INITIAL>"fi" {
	beginToken(yytext);
	return T_FI;
}
<INITIAL>"for" {
	beginToken(yytext);
	return T_FOR;
}
<INITIAL>"do" {
	beginToken(yytext);
	return T_DO;
}
<INITIAL>"done" {
	beginToken(yytext);
	return T_DONE;
}
<INITIAL>"while" {
	beginToken(yytext);
	return T_WHILE;
}

<INITIAL,CMDEXPR>[0-9]+ {
	beginToken(yytext);
	yylval.intval = atoi(yytext);
	return T_NUMBER;
}

<INITIAL,CMDEXPR>\$[a-zA-Z_0-9]+ {
	beginToken(yytext);
	yylval.strval = strdup(yytext + 1);
	return T_VAR;
}

<INITIAL>\" {
	beginToken(yytext);
	BEGIN(DCONSTSTR);
	return *yytext;
}

<DCONSTSTR>\" {
	beginToken(yytext);
	BEGIN(INITIAL);
	return *yytext;
}

<DCONSTSTR>"{" {
	beginToken(yytext);
	BEGIN(CMDEXPR);
	return *yytext;
}

<DCONSTSTR>[^\"\{]+ {
	beginToken(yytext);
	yylval.strval = strdup(yytext);
	return T_STRING;
}

<CMDEXPR>"}" {
	beginToken(yytext);
	BEGIN(DCONSTSTR);
	return *yytext;
}

<INITIAL,CMDEXPR>'[^']*' {
	beginToken(yytext);
	yylval.strval = strndup(yytext + 1,strlen(yytext) - 2);
	return T_STRING_SCONST;
}

<INITIAL,CMDEXPR>"<=" {
	beginToken(yytext);
	return T_LEQ;
}
<INITIAL,CMDEXPR>">=" {
	beginToken(yytext);
	return T_GEQ;
}
<INITIAL,CMDEXPR>"==" {
	beginToken(yytext);
	return T_EQ;
}
<INITIAL,CMDEXPR>"!=" {
	beginToken(yytext);
	return T_NEQ;
}
<INITIAL,CMDEXPR>">>" {
	beginToken(yytext);
	return T_APPEND;
}
<INITIAL,CMDEXPR>"2>&1" {
	beginToken(yytext);
	return T_ERR2OUT;
}
<INITIAL,CMDEXPR>"1>&2" {
	beginToken(yytext);
	return T_OUT2ERR;
}

<INITIAL,CMDEXPR>"`"|"&"|";"|"|"|"=" {
	beginToken(yytext);
	return *yytext;
}
<INITIAL,CMDEXPR>">"|"<" {
	beginToken(yytext);
	return *yytext;
}
<INITIAL,CMDEXPR>"+"|"-"|"*"|"/"|"%"|"^" {
	beginToken(yytext);
	return *yytext;
}
<INITIAL,CMDEXPR>"("|")" {
	beginToken(yytext);
	return *yytext;
}
<INITIAL>"{"|"}" {
	beginToken(yytext);
	return *yytext;
}

<INITIAL,CMDEXPR>[ \t\n]+ {
	/* eat up whitespace */
	beginToken(yytext);
}

<INITIAL,CMDEXPR>[a-zA-Z0-9_\-!?:~,\.]+ {
	beginToken(yytext);
	yylval.strval = strdup(yytext);
	return T_STRING;
}

<INITIAL,CMDEXPR>. {
	beginToken(yytext);
	yyerror("Unrecognized character %c",*yytext);
	BEGIN(INITIAL);
}
