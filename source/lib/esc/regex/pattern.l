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

<INITIAL>"(" {
	return T_GROUP_BEGIN;
}
<INITIAL>")" {
	return T_GROUP_END;
}

<INITIAL>"[" {
	yy_push_state(CHARCLASS);
	return T_CHARCLASS_BEGIN;
}
<INITIAL,CHARCLASS>"]" {
	if(yy_start_stack_ptr > 0)
		yy_pop_state();
	return T_CHARCLASS_END;
}

<INITIAL>"{" {
	yy_push_state(REPSPEC);
	return T_REPSPEC_BEGIN;
}
<INITIAL,REPSPEC>"}" {
	if(yy_start_stack_ptr > 0)
		yy_pop_state();
	return T_REPSPEC_END;
}

<REPSPEC>[0-9]+ {
	yylval.number = atoi(yytext);
	return T_NUMBER;
}
<REPSPEC>"," {
	return T_COMMA;
}

<INITIAL,CHARCLASS>"^" {
	return T_NEGATE;
}
<CHARCLASS>"-" {
	return T_RANGE;
}
<INITIAL>"." {
	return T_DOT;
}

<INITIAL>"*" {
	return T_REP_ANY;
}
<INITIAL>"+" {
	return T_REP_ONEPLUS;
}
<INITIAL>"?" {
	return T_REP_OPTIONAL;
}

<INITIAL,CHARCLASS>[^\(\)\[\]\{\}\*\+\?\.] {
	yylval.character = *yytext;
	return T_CHAR;
}
