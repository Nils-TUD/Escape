/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sys/common.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "lang.h"
#include "parser.h"

extern YYLTYPE yylloc;
extern YYLTYPE yylastloc;
extern int yycolumn;
extern int openGraves;
extern int yylineno;
static bool interrupted = false;

void lang_reset(void) {
	openGraves = 0;
	interrupted = false;
	yylloc.filename = NULL;
	yylloc.first_line = 1;
	yylloc.first_column = 1;
	yylloc.last_line = 1;
	yylloc.last_column = 1;
	*yylloc.line = '\0';
	yylineno = 1;
	yycolumn = 1;
}

void lang_setInterrupted(void) {
	interrupted = true;
}

bool lang_isInterrupted(void) {
	return interrupted;
}

static void vlyyerror(YYLTYPE t,const char *s,va_list ap) {
	ssize_t i;
	if(t.first_line)
		fprintf(stderr,"%s:%d: ",t.filename,t.first_line);
	vfprintf(stderr,s,ap);
	fprintf(stderr,":\n");
	fprintf(stderr,"  %s\n",t.line);
	fprintf(stderr,"  %*s",t.first_column - 1,"");
	for(i = t.last_column - t.first_column; i >= 0; i--)
		putc('^',stderr);
	putc('\n',stderr);
}

void yyerror(const char *s,...) {
	va_list ap;
	va_start(ap,s);
	if(strspn(yylloc.line," \t\n\r") == strlen(yylloc.line))
		vlyyerror(yylastloc,s,ap);
	else
		vlyyerror(yylloc,s,ap);
	va_end(ap);
}
