/* Reverse polish notation calculator.  */
 
%{
  #define YYSTYPE int
  int yylex (void);
  void yyerror (char const *);
  int pow(int a,int b);
%}
 
%token NUM
%left '-' '+'
%left '*' '/'
%left NEG     /* negation--unary minus */
%right '^'    /* exponentiation */

%% /* Grammar rules and actions follow.  */

input:    /* empty */
         | input line
;
 
line:     '\n'
         | exp '\n'      { printf ("\t%d\n", $1); }
         
;
 
exp:      NUM          				{ $$ = $1;           }
         | exp '+' exp  			{ $$ = $1 + $3;      }
         | exp '-' exp  			{ $$ = $1 - $3;      }
         | exp '*' exp   			{ $$ = $1 * $3;      }
         | exp '/' exp   			{ $$ = $1 / $3;      }
         | '-' exp  %prec NEG { $$ = -$2;        }
         | exp '^' exp        { $$ = pow($1, $3); }
         | '(' exp ')'        { $$ = $2;         }
;

%%
