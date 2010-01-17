/* required for us! */
%option noyywrap

%{
	#include "parser.h"
	#include "lang.h"
	#include <string.h>
%}

%s COMMENT

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

<INITIAL>[0-9]+ {
	beginToken(yytext);
	yylval.intval = atoi(yytext);
	return T_NUMBER;
}

<INITIAL>\$[a-zA-Z_0-9]+ {
	beginToken(yytext);
	yylval.strval = strdup(yytext + 1);
	return T_VAR;
}

<INITIAL>\"[^\"]*\" {
	beginToken(yytext);
	yylval.strval = strndup(yytext + 1,strlen(yytext) - 2);
	return T_STRING_DCONST;
}
<INITIAL>'[^']*' {
	beginToken(yytext);
	yylval.strval = strndup(yytext + 1,strlen(yytext) - 2);
	return T_STRING_SCONST;
}

<INITIAL>"<=" {
	beginToken(yytext);
	return T_LEQ;
}
<INITIAL>">=" {
	beginToken(yytext);
	return T_GEQ;
}
<INITIAL>"==" {
	beginToken(yytext);
	return T_EQ;
}
<INITIAL>"!=" {
	beginToken(yytext);
	return T_NEQ;
}
<INITIAL>">>" {
	beginToken(yytext);
	return T_APPEND;
}
<INITIAL>"2>&1" {
	beginToken(yytext);
	return T_ERR2OUT;
}
<INITIAL>"1>&2" {
	beginToken(yytext);
	return T_OUT2ERR;
}

<INITIAL>"`"|"&"|";"|"|"|"=" {
	beginToken(yytext);
	return *yytext;
}
<INITIAL>">"|"<" {
	beginToken(yytext);
	return *yytext;
}
<INITIAL>"+"|"-"|"*"|"/"|"%"|"^" {
	beginToken(yytext);
	return *yytext;
}
<INITIAL>"("|")"|"{"|"}" {
	beginToken(yytext);
	return *yytext;
}

<INITIAL>[ \t\n]+ {
	/* eat up whitespace */
	beginToken(yytext);
}

<INITIAL>[a-zA-Z0-9_\-!?:~,\.]+ {
	beginToken(yytext);
	yylval.strval = strdup(yytext);
	return T_STRING;
}

<INITIAL>. {
	beginToken(yytext);
	printf("Unrecognized character %c\n",*yytext);
}
