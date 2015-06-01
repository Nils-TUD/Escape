%{
	#include <stdio.h>
	#include <stdlib.h>

	#include "pattern.h"

	int yylex (void);
	void yyerror (char const *);

	extern const char *regex_patstr;
	extern void *regex_result;
%}

%union {
	int number;
	char character;
	void *node;
}

%token <character> T_CHAR
%token <number> T_NUMBER
%token T_GROUP_BEGIN
%token T_GROUP_END
%token T_CHARCLASS_BEGIN
%token T_CHARCLASS_END
%token T_REPSPEC_BEGIN
%token T_REPSPEC_END
%token T_NEGATE
%token T_DOT
%token T_COMMA
%token T_RANGE
%token T_CHOICE
%token T_REP_ANY
%token T_REP_ONEPLUS
%token T_REP_OPTIONAL

%type <node> regex elemlist elem charclass_list charclass_elem choice_list

%destructor { pattern_destroy($$); } <node>

%%

regex:	elemlist										{ regex_result = pattern_createGroup($1); $$ = NULL; }
;

elemlist:
	elemlist elem										{ $$ = $1; pattern_addToList($1,$2); }
	| /* empty */										{ $$ = pattern_createList(true); }
;

elem:
	T_CHAR 												{ $$ = pattern_createChar($1); }
	| T_DOT												{ $$ = pattern_createDot(); }
	| T_GROUP_BEGIN elemlist T_GROUP_END				{ $$ = pattern_createGroup($2); }
	| T_CHARCLASS_BEGIN
			charclass_list T_CHARCLASS_END				{ $$ = pattern_createCharClass($2,false); }
	| T_CHARCLASS_BEGIN
			T_NEGATE charclass_list T_CHARCLASS_END		{ $$ = pattern_createCharClass($3,true); }
	| choice_list										{ $$ = pattern_createChoice($1); }
	| elem T_REP_ANY									{ $$ = pattern_createRepeat($1,0,1 << 30); }
	| elem T_REP_ONEPLUS								{ $$ = pattern_createRepeat($1,1,1 << 30); }
	| elem T_REP_OPTIONAL								{ $$ = pattern_createRepeat($1,0,1); }
	| elem T_REPSPEC_BEGIN
			T_NUMBER T_COMMA T_NUMBER
			T_REPSPEC_END								{ $$ = pattern_createRepeat($1,$3,$5); }
	| elem T_REPSPEC_BEGIN
			T_NUMBER T_COMMA
			T_REPSPEC_END								{ $$ = pattern_createRepeat($1,$3,1 << 30); }
	| elem T_REPSPEC_BEGIN T_NUMBER T_REPSPEC_END		{ $$ = pattern_createRepeat($1,$3,$3); }
;

choice_list:
	choice_list T_CHOICE elem							{ $$ = $1; pattern_addToList($1,$3); }
	| elem												{ $$ = pattern_createList(false); pattern_addToList($$,$1); }
;

charclass_list:
	charclass_list charclass_elem						{ $$ = $1; pattern_addToCharClassList($1,$2); }
	| /* empty */										{ $$ = pattern_createCharClassList(); }
;

charclass_elem:
	T_CHAR												{ $$ = pattern_createCharClassElem($1,$1); }
	| T_CHAR T_RANGE T_CHAR								{ $$ = pattern_createCharClassElem($1,$3); }
;

%%
