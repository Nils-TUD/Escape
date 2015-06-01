/* required for us! */
%option noyywrap
%option stack

%{
	#include "pattern-parse.h"

	extern const char *regex_patstr;

	#ifndef YY_BUF_SIZE
	#	define YY_BUF_SIZE 16
	#endif

	#define YY_INPUT(buf,result,max_size) \
		{ \
			int c = *regex_patstr++; \
			result = (c == '\0') ? YY_NULL : (buf[0] = c, 1); \
		}
%}

%s REPSPEC
%s CHARCLASS

%%

 /* groups */
<INITIAL>"(" {
	return T_GROUP_BEGIN;
}
<INITIAL>")" {
	return T_GROUP_END;
}

 /* character classes */
<INITIAL>"[" {
	yy_push_state(CHARCLASS);
	return T_CHARCLASS_BEGIN;
}
 /* we want to accept it in INITIAL, too, to detect errors */
 /* but then we need to make sure to only pop the state if we are in CHARCLASS */
 /* fortunately, it is not nested, so that it suffices to check whether we are not in INITIAL */
<INITIAL,CHARCLASS>"]" {
	if(yy_start_stack_ptr > 0)
		yy_pop_state();
	return T_CHARCLASS_END;
}

 /* repeat specification */
<INITIAL>"{" {
	yy_push_state(REPSPEC);
	return T_REPSPEC_BEGIN;
}
 /* same as above */
<INITIAL,REPSPEC>"}" {
	if(yy_start_stack_ptr > 0)
		yy_pop_state();
	return T_REPSPEC_END;
}

 /* these have only special meaning in the repeat specification */
<REPSPEC>[0-9]+ {
	yylval.number = atoi(yytext);
	return T_NUMBER;
}
<REPSPEC>"," {
	return T_COMMA;
}

 /* other special characters */
<INITIAL,CHARCLASS>"^" {
	return T_NEGATE;
}
<CHARCLASS>"-" {
	return T_RANGE;
}
<INITIAL>"|" {
	return T_CHOICE;
}
<INITIAL>"." {
	return T_DOT;
}

 /* repeat operators */
<INITIAL>"*" {
	return T_REP_ANY;
}
<INITIAL>"+" {
	return T_REP_ONEPLUS;
}
<INITIAL>"?" {
	return T_REP_OPTIONAL;
}

 /* escaping */
<INITIAL,CHARCLASS>\\[\(\)\[\]\{\}\*\+\?\.\|] {
	yylval.character = yytext[1];
	return T_CHAR;
}
<CHARCLASS>\\[-\^] {
	yylval.character = yytext[1];
	return T_CHAR;
}

 /* all other stuff are simply characters */
<INITIAL,CHARCLASS,REPSPEC>. {
	yylval.character = *yytext;
	return T_CHAR;
}
