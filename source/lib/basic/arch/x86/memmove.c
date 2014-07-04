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

#include <stddef.h>
#include <string.h>

/* this is necessary to prevent that gcc transforms a loop into library-calls
 * (which might lead to recursion here) */
#pragma GCC optimize ("no-tree-loop-distribute-patterns")

void *memmove(void *dest,const void *src,size_t count) {
	uchar *s,*d;
	/* nothing to do? */
	if((uchar*)dest == (uchar*)src || count == 0)
		return dest;

	/* moving forward */
	if((uintptr_t)dest > (uintptr_t)src) {
		ulong *dsrc = (ulong*)((uintptr_t)src + count - sizeof(ulong));
		ulong *ddest = (ulong*)((uintptr_t)dest + count - sizeof(ulong));
		while(count >= sizeof(ulong)) {
			*ddest-- = *dsrc--;
			count -= sizeof(ulong);
		}
		s = (uchar*)dsrc + (sizeof(ulong) - 1);
		d = (uchar*)ddest + (sizeof(ulong) - 1);
		while(count-- > 0)
			*d-- = *s--;
	}
	/* moving backwards */
	else
		memcpy(dest,src,count);

	return dest;
}
