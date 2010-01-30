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

#include <esc/signals.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/dir.h>
#include "parser.h"
#include "exec/running.h"

extern tFile *yyin;
extern int yydebug;
extern int yyparse(void);

int main(int argc,char *argv[]) {
	char abs[MAX_PATH_LEN];
	yydebug = 0;
	/* skip over program name */
	++argv, --argc;
	if (argc > 0) {
		abspath(abs,MAX_PATH_LEN,argv[0]);
		yyin = fopen(abs,"r");
	}
	else
		yyin = stdin;
	run_init();
	/* TODO call run_gc() after each script-execution */
	return yyparse();
}
