%{
	#include <stdio.h>
	#include <stdlib.h>

	#include "pattern.h"

	int yylex (void);
	void yyerror (char const *);

	extern const char *regex_patstr;
	extern void *regex_result;
	extern int regex_flags;
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
%token T_END
%token T_DOT
%token T_COMMA
%token T_RANGE
%token T_CHOICE
%token T_REP_ANY
%token T_REP_ONEPLUS
%token T_REP_OPTIONAL
%token T_DIGIT_CLS
%token T_NON_DIGIT_CLS
%token T_WS_CLS
%token T_NON_WS_CLS
%token T_WORD_CLS
%token T_NON_WORD_CLS

%type <node> regex elemlist elem std_elem charclass_list charclass_elem choice_list charclass_abrv

%destructor { pattern_destroy($$); } <node>

%%

regex:
	elemlist											{ regex_result = pattern_createGroup($1); $$ = NULL; }
	| T_NEGATE elemlist									{
															regex_flags = REGEX_FLAG_BEGIN;
															regex_result = pattern_createGroup($2);
															$$ = NULL;
														}
	| elemlist T_END									{
															regex_flags = REGEX_FLAG_END;
															regex_result = pattern_createGroup($1);
															$$ = NULL;
														}
	| T_NEGATE elemlist T_END							{
															regex_flags = REGEX_FLAG_BEGIN | REGEX_FLAG_END;
															regex_result = pattern_createGroup($2);
															$$ = NULL;
														}
;

elemlist:
	elemlist elem										{ $$ = $1; pattern_addToList($1,$2); }
	| /* empty */										{ $$ = pattern_createList(true); }
;

elem:
	std_elem											{ $$ = $1; }
	| std_elem T_CHOICE choice_list						{ pattern_addToList($3,$1); $$ = pattern_createChoice($3); }
;

std_elem:
	T_CHAR 												{ $$ = pattern_createChar($1); }
	| T_DOT												{ $$ = pattern_createDot(); }
	| T_GROUP_BEGIN elemlist T_GROUP_END				{ $$ = pattern_createGroup($2); }
	| T_CHARCLASS_BEGIN
			charclass_list T_CHARCLASS_END				{ $$ = pattern_createCharClass($2,false); }
	| T_CHARCLASS_BEGIN
			T_NEGATE charclass_list T_CHARCLASS_END		{ $$ = pattern_createCharClass($3,true); }
	| std_elem T_REP_ANY								{ $$ = pattern_createRepeat($1,0,1 << 30); }
	| std_elem T_REP_ONEPLUS							{ $$ = pattern_createRepeat($1,1,1 << 30); }
	| std_elem T_REP_OPTIONAL							{ $$ = pattern_createRepeat($1,0,1); }
	| std_elem T_REPSPEC_BEGIN
			T_NUMBER T_COMMA T_NUMBER
			T_REPSPEC_END								{ $$ = pattern_createRepeat($1,$3,$5); }
	| std_elem T_REPSPEC_BEGIN
			T_NUMBER T_COMMA
			T_REPSPEC_END								{ $$ = pattern_createRepeat($1,$3,1 << 30); }
	| std_elem T_REPSPEC_BEGIN
			T_NUMBER
			T_REPSPEC_END								{ $$ = pattern_createRepeat($1,$3,$3); }
	| charclass_abrv									{ $$ = $1; }
;

choice_list:
	choice_list T_CHOICE std_elem						{ $$ = $1; pattern_addToList($1,$3); }
	| std_elem											{ $$ = pattern_createList(false); pattern_addToList($$,$1); }
;

charclass_list:
	charclass_list charclass_elem						{ $$ = $1; pattern_addToList($1,$2); }
	| /* empty */										{ $$ = pattern_createList(false); }
;

charclass_elem:
	T_CHAR												{ $$ = pattern_createCharClassElem($1,$1); }
	| T_CHAR T_RANGE T_CHAR								{ $$ = pattern_createCharClassElem($1,$3); }
	| charclass_abrv									{ $$ = $1; }
	| T_CHARCLASS_BEGIN
			charclass_list T_CHARCLASS_END				{ $$ = pattern_createCharClass($2,false); }
	| T_CHARCLASS_BEGIN
			T_NEGATE charclass_list T_CHARCLASS_END		{ $$ = pattern_createCharClass($3,true); }
;

charclass_abrv:
	T_DIGIT_CLS											{ $$ = pattern_createSimpleCharClass('0','9',false); }
	| T_NON_DIGIT_CLS									{ $$ = pattern_createSimpleCharClass('0','9',true); }
	| T_WS_CLS											{ $$ = pattern_createWSClass(false); }
	| T_NON_WS_CLS										{ $$ = pattern_createWSClass(true); }
	| T_WORD_CLS										{ $$ = pattern_createWordClass(false); }
	| T_NON_WORD_CLS									{ $$ = pattern_createWordClass(true); }
;

%%
