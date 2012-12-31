%{
	#include <stdio.h>
	#include <stdlib.h>
	#include "gcalc.h"
	int yylex (void);
	void yyerror (char const *);
%}

%token T_NUMBER
%left '+' '-'
%left '*' '/' '%'
%left T_NEG											/* negation--unary minus */

%%

input:		exp										{ callback($1); }
;

exp:		T_NUMBER								{ $$ = $1; }
			| exp '+' exp							{ $$ = $1 + $3; }
			| exp '-' exp							{ $$ = $1 - $3; }
			| exp '*' exp							{ $$ = $1 * $3; }
			| exp '/' exp							{ $$ = $3 == 0 ? 0 : $1 / $3; }
			| exp '%' exp							{ $$ = (long)$3 == 0 ? 0 : (long)$1 % (long)$3; }
			| '-' exp  %prec T_NEG					{ $$ = -$2; }
			| '(' exp ')'							{ $$ = $2; }
;

%%
