/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <assert.h>
#include <string.h>

char *strncpy(char *to,const char *from,size_t count) {
	char *res = to;

	vassert(from != NULL,"from == NULL");
	vassert(to != NULL,"to == NULL");

	/* copy source string */
	while(*from && count > 0) {
		*to++ = *from++;
		count--;
	}
	/* terminate & fill with \0 if needed */
	while(count-- > 0)
		*to++ = '\0';
	return res;
}
