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

void beginToken(char *t) {
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
void yyerror(char const *s) {
	fprintf(stderr,"Line %d: %s\n",yylloc.first_line,s);
}
