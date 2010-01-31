/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <esc/common.h>
#include <esc/fileio.h>
#include <string.h>
#include <stdarg.h>
#include "lang.h"

typedef struct {
	s32 first_line;
	s32 last_line;
	s32 first_column;
	s32 last_column;
} sYYLoc;

static sYYLoc yylloc;
static s32 nCol = 1;
static s32 nRow = 1;
static bool interrupted = false;

void lang_reset(void) {
	nCol = 1;
	nRow = 1;
	interrupted = false;
}

void lang_setInterrupted(void) {
	interrupted = true;
}

bool lang_isInterrupted(void) {
	return interrupted;
}

void lang_beginToken(char *t) {
	u32 len = strlen(t);
	if(*t == '\n') {
		nRow++;
		nCol = 1;
	}
	else
		nCol += len;

	yylloc.first_line = nRow;
	yylloc.first_column = nCol;
	yylloc.last_line = nRow;
	yylloc.last_column = nCol + len - 1;
}

/* Called by yyparse on error.  */
void yyerror(char const *s,...) {
	va_list l;
	fprintf(stderr,"Line %d, Column %d: ",yylloc.first_line,yylloc.first_column);
	va_start(l,s);
	vfprintf(stderr,s,l);
	va_end(l);
	fprintf(stderr,"\n");
}
