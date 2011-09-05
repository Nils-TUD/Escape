%option noyywrap
%option yylineno

%{
/**
 * $Id: scanner.l 239 2011-08-30 18:28:06Z nasmussen $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "common.h"
#include "cli/console.h"
#include "cli/lang/specreg.h"
#include "cli/lang/symmap.h"
#include "mmix/mem.h"
#include "parser.h"
#include "mmix/io.h"
%}

%{
	void yyerror(const char *s,...);
	void yyerror(const char *s,...) {
		va_list ap;
		va_start(ap,s);
		cons_error(s,ap);
		va_end(ap);
	}

	#define YY_INPUT(buf,result,max_size)	((result) = cons_getc((buf)));
	#define YY_USER_ACTION								cons_tokenAction();
%}

L				[A-Za-z_]
D				[0-9]
H				[0-9A-Fa-f]
ID				{L}({L}|{D}|\.)*
BINNUM			0b[01]+
OCTNUM			0{D}+
DECNUM			0|([1-9]{D}*)
HEXNUM			(#|0x){H}+
SPECIALREG		r[A-Z](B|S|T|[W-Z])?
FLOAT			{D}*\.{D}+([eE][-+]?{D}+)?

%%

[\ \t\r]+	{
	return T_WS;
}

\n+ {
	return T_NL;
}

{BINNUM} {
	yylval.intval = mstrtoo(yytext + 2,NULL,2);
	return T_INTEGER;
}
{OCTNUM} {
	yylval.intval = mstrtoo(yytext + 1,NULL,8);
	return T_INTEGER;
}
{DECNUM} {
	yylval.intval = mstrtoo(yytext,NULL,10);
	return T_INTEGER;
}
{HEXNUM} {
	yylval.intval = mstrtoo(*yytext == '#' ? yytext + 1 : yytext + 2,NULL,16);
	return T_INTEGER;
}
{SPECIALREG} {
	yylval.intval = sreg_getByName(yytext);
	return T_INTEGER;
}
{FLOAT} {
	yylval.floatval = strtod(yytext,NULL);
	return T_FLOAT;
}

M {
	return T_VMEM;
}
M1 {
	return T_VMEM1;
}
M2 {
	return T_VMEM2;
}
M4 {
	return T_VMEM4;
}
M8 {
	return T_VMEM8;
}
m {
	return T_PMEM;
}
l {
	return T_LREG;
}
g {
	return T_GREG;
}
sp {
	return T_SREG;
}
\$ {
	return T_DREG;
}
\@ {
	return T_AT;
}
\${DECNUM} {
	yylval.intval = mstrtoo(yytext + 1,NULL,10);
	return T_DREGLIT;
}

\[ {
	return T_LBR;
}
\] {
	return T_RBR;
}
\( {
	return T_LPAR;
}
\) {
	return T_RPAR;
}

: {
	return T_COLON;
}
\.\. {
	return T_DOTDOT;
}

\+ {
	return T_ADD;
}
\- {
	return T_SUB;
}
\* {
	return T_MULU;
}
s\* {
	return T_MUL;
}
\/ {
	return T_DIVU;
}
s\/ {
	return T_DIV;
}
\% {
	return T_MODU;
}
s\% {
	return T_MOD;
}
\>\> {
	return T_SAR;
}
\<\< {
	return T_SL;
}
\>\>\> {
	return T_SR;
}
\& {
	return T_AND;
}
\| {
	return T_OR;
}
\^ {
	return T_XOR;
}
\~ {
	return T_NOT;
}

{ID} {
	if(strlen(yytext) > 3) {
		octa addr = symmap_getAddress(yytext);
		if(addr != 0) {
			yylval.intval = addr;
			return T_INTEGER;
		}
	}
	yylval.strval = (char*)mem_alloc(strlen(yytext) + 1);
	strcpy(yylval.strval,yytext);
	return T_IDENT;
}

. {
	yyerror("Unrecognized character %c (%02x)",*yytext,*yytext);
}

%%


void cons_showToken(int token);
void cons_showToken(int token) {
	mprintf("TOKEN = ");
	switch(token) {
		case 0:
			mprintf("-- EOF --");
			break;
		case T_WS:
			mprintf("T_WS in line %d",yylloc.first_line);
			break;
		case T_NL:
			mprintf("T_NL in line %d",yylloc.first_line);
			break;
		case T_INTEGER:
			mprintf("T_INTEGER (#%OX) in line %d",yylval.intval,yylloc.first_line);
			break;
		case T_FLOAT:
			mprintf("T_FLOAT (%g) in line %d",yylval.floatval,yylloc.first_line);
			break;
		case T_VMEM:
			mprintf("T_VMEM in line %d",yylloc.first_line);
			break;
		case T_VMEM1:
			mprintf("T_VMEM1 in line %d",yylloc.first_line);
			break;
		case T_VMEM2:
			mprintf("T_VMEM2 in line %d",yylloc.first_line);
			break;
		case T_VMEM4:
			mprintf("T_VMEM4 in line %d",yylloc.first_line);
			break;
		case T_VMEM8:
			mprintf("T_VMEM8 in line %d",yylloc.first_line);
			break;
		case T_PMEM:
			mprintf("T_PMEM in line %d",yylloc.first_line);
			break;
		case T_DREG:
			mprintf("T_DREG in line %d",yylloc.first_line);
			break;
		case T_DREGLIT:
			mprintf("T_DREGLIT (%d) in line %d",yylval.intval,yylloc.first_line);
			break;
		case T_LREG:
			mprintf("T_LREG in line %d",yylloc.first_line);
			break;
		case T_GREG:
			mprintf("T_GREG in line %d",yylloc.first_line);
			break;
		case T_AT:
			mprintf("T_AT in line %d",yylloc.first_line);
			break;
		case T_LBR:
			mprintf("T_LBR in line %d",yylloc.first_line);
			break;
		case T_RBR:
			mprintf("T_RBR in line %d",yylloc.first_line);
			break;
		case T_LPAR:
			mprintf("T_LPAR in line %d",yylloc.first_line);
			break;
		case T_RPAR:
			mprintf("T_RPAR in line %d",yylloc.first_line);
			break;
		case T_COLON:
			mprintf("T_COLON in line %d",yylloc.first_line);
			break;
		case T_DOTDOT:
			mprintf("T_DOTDOT in line %d",yylloc.first_line);
			break;
		case T_ADD:
			mprintf("T_ADD in line %d",yylloc.first_line);
			break;
		case T_SUB:
			mprintf("T_SUB in line %d",yylloc.first_line);
			break;
		case T_MUL:
			mprintf("T_MUL in line %d",yylloc.first_line);
			break;
		case T_MULU:
			mprintf("T_MULU in line %d",yylloc.first_line);
			break;
		case T_DIV:
			mprintf("T_DIV in line %d",yylloc.first_line);
			break;
		case T_DIVU:
			mprintf("T_DIVU in line %d",yylloc.first_line);
			break;
		case T_MOD:
			mprintf("T_MOD in line %d",yylloc.first_line);
			break;
		case T_MODU:
			mprintf("T_MODU in line %d",yylloc.first_line);
			break;
		case T_SL:
			mprintf("T_SL in line %d",yylloc.first_line);
			break;
		case T_SR:
			mprintf("T_SR in line %d",yylloc.first_line);
			break;
		case T_SAR:
			mprintf("T_SAR in line %d",yylloc.first_line);
			break;
		case T_OR:
			mprintf("T_OR in line %d",yylloc.first_line);
			break;
		case T_AND:
			mprintf("T_AND in line %d",yylloc.first_line);
			break;
		case T_XOR:
			mprintf("T_XOR in line %d",yylloc.first_line);
			break;
		case T_NOT:
			mprintf("T_NOT in line %d",yylloc.first_line);
			break;
		case T_IDENT:
			mprintf("T_IDENT (%s) in line %d",yylval.strval,yylloc.first_line);
			break;
		default:
			/* this should never happen */
			assert(false);
			break;
	}
	mprintf("\n");
}

