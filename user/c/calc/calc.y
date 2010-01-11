%{
	#include <stdio.h>
	#define YYSTYPE int
	int yylex (void);
	void yyerror (char const *);
	int pow(int a,int b);
%}

%token T_NUMBER
%left '+' '-'
%left '*' '/' '%'
%left T_NEG											/* negation--unary minus */
%right '^'											/* exponentiation */

%%

input:	/* empty */
				| input line						{ printf("> "); }
				| error '\n'						{ printf("> "); yyerrok; }
;

line:		'\n'
				| exp '\n'							{ printf ("\t%d\n", $1); } 
;

exp:		T_NUMBER								{ $$ = $1; }
			| exp '+' exp							{ $$ = $1 + $3; }
			| exp '-' exp							{ $$ = $1 - $3; }
			| exp '*' exp							{ $$ = $1 * $3; }
			| exp '/' exp							{ $$ = $1 / $3; }
			| exp '%' exp							{ $$ = $1 % $3; }
			| '-' exp  %prec T_NEG		{ $$ = -$2; }
			| exp '^' exp							{ $$ = pow($1, $3); }
			| '(' exp ')'							{ $$ = $2; }
;

%%
