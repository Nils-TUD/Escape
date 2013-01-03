/* required for us! */
%option noyywrap

%{
	#include <string.h>
	#include <stdio.h>
	#include "gcalc.h"
	#include "parser.h"
%}

%{
	#define YY_INPUT(buf,result,max_size) \
		{ \
			int c = *parse_text++; \
			result = (c == '\0') ? YY_NULL : (buf[0] = c, 1); \
		}
%}

%%

[0-9\.]+ {
	yylval = strtod(yytext,NULL);
	return T_NUMBER;
}

"+"|"-"|"*"|"/"|"%"|"("|")" {
	return *yytext;
}

[ \t]+	/* eat up whitespace */

. {
	fprintf(stderr,"Error: Unrecognized input: '%s'\n",yytext);
}
