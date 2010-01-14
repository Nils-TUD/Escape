/* required for us! */
%option noyywrap

%{
	#include "parser.h"
	#include <string.h>
%}

%%

"if" {
	return T_IF;
}
"then" {
	return T_THEN;
}
"else" {
	return T_ELSE;
}
"fi" {
	return T_FI;
}

[0-9]+ {
	yylval.intval = atoi(yytext);
	return T_NUMBER;
}

\$[a-zA-Z_0-9]+ {
	yylval.strval = strdup(yytext + 1);
	return T_VAR;
}

\"[^\"]*\" {
	yylval.strval = strndup(yytext + 1,strlen(yytext) - 2);
	return T_STRING_DCONST;
}
'[^']*' {
	yylval.strval = strndup(yytext + 1,strlen(yytext) - 2);
	return T_STRING_SCONST;
}

"<=" {
	return T_LEQ;
}
">=" {
	return T_GEQ;
}
"==" {
	return T_EQ;
}
"!=" {
	return T_NEQ;
}
">>" {
	return T_APPEND;
}
"2>&1" {
	return T_ERR2OUT;
}
"1>&2" {
	return T_OUT2ERR;
}

"`"|"&"|";"|"|"|"=" {
	return *yytext;
}
">"|"<" {
	return *yytext;
}
"+"|"-"|"*"|"/"|"%"|"^" {
	return *yytext;
}
"("|")"|"{"|"}" {
	return *yytext;
}

[ \t\n]+			/* eat up whitespace */

[a-zA-Z0-9_\-!?~;,\.]+ {
	yylval.strval = strdup(yytext);
	return T_STRING;
}

. {
	printf("Unrecognized character %c\n",*yytext);
}
