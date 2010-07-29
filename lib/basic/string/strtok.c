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

#include <stddef.h>
#include <string.h>

char *strtok(char *str,const char *delimiters) {
	static char *last = NULL;
	char *res;
	u32 start,end;
	if(str)
		last = str;
	if(last == NULL)
		return NULL;

	/* find first char of the token */
	start = strspn(last,delimiters);
	last += start;
	/* reached end? */
	if(*last == '\0') {
		last = NULL;
		return NULL;
	}
	/* store beginning */
	res = last;

	/* find end of the token */
	end = strcspn(last,delimiters);
	/* have we reached the end? */
	if(last[end] == '\0')
		last = NULL;
	else {
		last[end] = '\0';
		/* we want to continue at that place next time */
		last += end + 1;
	}

	return res;
}
