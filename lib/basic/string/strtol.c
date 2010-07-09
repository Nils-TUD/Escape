/**
 * $Id: strtol.c 650 2010-05-06 19:05:10Z nasmussen $
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
#include <ctype.h>
#include <string.h>
#include <assert.h>

s32 strtol(const char *nptr,char **endptr,s32 base) {
	s32 i = 0;
	bool neg = false;
	char c;
	assert(nptr);
	assert(base == 0 || (base >= 2 && base <= 36));

	/* skip leading whitespace */
	while(isspace(*nptr))
		nptr++;

	/* optional '-' or '+' */
	if(*nptr == '-') {
		neg = true;
		nptr++;
	}
	else if(*nptr == '+')
		nptr++;

	/* determine base */
	if((base == 16 || base == 0) && nptr[0] == '0' && nptr[1] == 'x') {
		nptr += 2;
		base = 16;
	}
	else if(base == 0 && nptr[0] == '0') {
		nptr++;
		base = 8;
	}
	if(base == 0)
		base = 10;

	/* read number */
	while((c = *nptr)) {
		char lc = tolower(c);
		/* check if its a digit that belongs to the number in specified base */
		if(base <= 10 && (!isdigit(c) || c - '0' >= base))
			break;
		if(base > 10 && (!isalnum(c) || (!isdigit(c) && (lc - 'a') >= base - 10)))
			break;
		if(lc >= 'a')
			i = i * base + (lc - 'a') + 10;
		else
			i = i * base + c - '0';
		nptr++;
	}

	/* switch sign? */
	if(neg)
		i = -i;
	/* store end */
	if(endptr)
		*endptr = (char*)nptr;
	return i;
}
