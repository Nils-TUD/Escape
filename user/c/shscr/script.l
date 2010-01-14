/* required for us! */
%option noyywrap

%{
	#include "parser.h"
	#include <string.h>
%}

%%

"if" {
	beginToken(yytext);
	return T_IF;
}
"then" {
	beginToken(yytext);
	return T_THEN;
}
"else" {
	beginToken(yytext);
	return T_ELSE;
}
"fi" {
	beginToken(yytext);
	return T_FI;
}

[0-9]+ {
	beginToken(yytext);
	yylval.intval = atoi(yytext);
	return T_NUMBER;
}

\$[a-zA-Z_0-9]+ {
	beginToken(yytext);
	yylval.strval = strdup(yytext + 1);
	return T_VAR;
}

\"[^\"]*\" {
	beginToken(yytext);
	yylval.strval = strndup(yytext + 1,strlen(yytext) - 2);
	return T_STRING_DCONST;
}
'[^']*' {
	beginToken(yytext);
	yylval.strval = strndup(yytext + 1,strlen(yytext) - 2);
	return T_STRING_SCONST;
}

"<=" {
	beginToken(yytext);
	return T_LEQ;
}
">=" {
	beginToken(yytext);
	return T_GEQ;
}
"==" {
	beginToken(yytext);
	return T_EQ;
}
"!=" {
	beginToken(yytext);
	return T_NEQ;
}
">>" {
	beginToken(yytext);
	return T_APPEND;
}
"2>&1" {
	beginToken(yytext);
	return T_ERR2OUT;
}
"1>&2" {
	beginToken(yytext);
	return T_OUT2ERR;
}

"`"|"&"|";"|"|"|"=" {
	beginToken(yytext);
	return *yytext;
}
">"|"<" {
	beginToken(yytext);
	return *yytext;
}
"+"|"-"|"*"|"/"|"%"|"^" {
	beginToken(yytext);
	return *yytext;
}
"("|")"|"{"|"}" {
	beginToken(yytext);
	return *yytext;
}
";" {
	beginToken(yytext);
	return *yytext;
}

[ \t\n]+ {
	/* eat up whitespace */
	beginToken(yytext);
}

[a-zA-Z0-9_\-!?:~,\.]+ {
	beginToken(yytext);
	yylval.strval = strdup(yytext);
	return T_STRING;
}

. {
	beginToken(yytext);
	printf("Unrecognized character %c\n",*yytext);
}
