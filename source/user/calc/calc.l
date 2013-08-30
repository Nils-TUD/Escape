/* required for us! */
%option noyywrap

%{
	#include "parser.h"
%}

%%

[0-9]+ {
	yylval = atoi(yytext);
	return T_NUMBER;
}

"+"|"-"|"*"|"/"|"%"|"^" {
	return *yytext;
}
"("|")" {
	return *yytext;
}
"\n" {
	return *yytext;
}

[ \t]+			/* eat up whitespace */
.						printf( "Unrecognized character: %s\n", yytext );
