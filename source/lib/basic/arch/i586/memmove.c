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
#include <string.h>

void *memmove(void *dest,const void *src,size_t count) {
	uchar *s,*d;
	/* nothing to do? */
	if((uchar*)dest == (uchar*)src || count == 0)
		return dest;

	/* moving forward */
	if((uintptr_t)dest > (uintptr_t)src) {
		uint *dsrc = (uint*)((uintptr_t)src + count - sizeof(uint));
		uint *ddest = (uint*)((uintptr_t)dest + count - sizeof(uint));
		while(count >= sizeof(uint)) {
			*ddest-- = *dsrc--;
			count -= sizeof(uint);
		}
		s = (uchar*)dsrc + (sizeof(uint) - 1);
		d = (uchar*)ddest + (sizeof(uint) - 1);
		while(count-- > 0)
			*d-- = *s--;
	}
	/* moving backwards */
	else
		memcpy(dest,src,count);

	return dest;
}
