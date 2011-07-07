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

#include <ctype.h>
#include <string.h>

void memswp(void *a,void *b,size_t n) {
	char *iptr = (char*)a;
	char *jptr = (char*)b;
	/* *iptr XOR *jptr == 0 for *iptr == *jptr ... */
	if(memcmp(iptr,jptr,n) != 0) {
		size_t x;
		for(x = 0; x < n; x++) {
			*iptr ^= *jptr;
			*jptr ^= *iptr;
			*iptr ^= *jptr;
			iptr++;
			jptr++;
		}
	}
}
