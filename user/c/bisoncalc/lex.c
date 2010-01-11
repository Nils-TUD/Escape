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

/* The lexical analyzer returns a double floating point
	number on the stack and the token NUM, or the numeric code
	of the character read if not a number.  It skips all blanks
	and tabs, and returns 0 for end-of-input.  */

#include <esc/common.h>
#include <stdio.h>
#include <ctype.h>

#include "parser.c"

int yylex (void)
{
	char c;

	/* Skip white space.  */
	while ((c = getchar ()) == ' ' || c == '\t')
		;
	/* Process numbers.  */
	if (isdigit (c))
	{
		ungetc (c, stdin);
		scanf ("%d", &yylval);
		return NUM;
	}
	/* Return end-of-input.  */
	if (c == EOF)
		return 0;
	/* Return a single char.  */
	return c;
}

