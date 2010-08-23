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
#include <assert.h>
#include <string.h>

char *itoa(char *target,u32 targetSize,s32 n) {
	char *s = target,*a = target,*b;

	assert(target != NULL);
	assert(targetSize >= 12);

	/* handle sign */
	if(n < 0) {
		*s++ = '-';
		a = s;
	}
	/* 0 is a special case */
	else if(n == 0)
		*s++ = '0';
	/* use negative numbers because otherwise we would get problems with -2147483648 */
	else
		n = -n;

	/* divide by 10 and put the remainer in the string */
	while(n < 0) {
		*s++ = -(n % 10) + '0';
		n = n / 10;
	}
	/* terminate */
	*s = '\0';

	/* reverse string */
	b = s - 1;
	while(a < b) {
		char t = *a;
		*a++ = *b;
		*b-- = t;
	}
	return target;
}
