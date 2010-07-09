/**
 * $Id: memmove.c 439 2010-02-02 00:23:24Z nasmussen $
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

void *memmove(void *dest,const void *src,u32 count) {
	u8 *s,*d;
	/* nothing to do? */
	if((u8*)dest == (u8*)src || count == 0)
		return dest;

	/* moving forward */
	if((u8*)dest > (u8*)src) {
		u32 *dsrc = (u32*)((u8*)src + count - sizeof(u32));
		u32 *ddest = (u32*)((u8*)dest + count - sizeof(u32));
		while(count >= sizeof(u32)) {
			*ddest-- = *dsrc--;
			count -= sizeof(u32);
		}
		s = (u8*)dsrc + (sizeof(u32) - 1);
		d = (u8*)ddest + (sizeof(u32) - 1);
		while(count-- > 0)
			*d-- = *s--;
	}
	/* moving backwards */
	else
		memcpy(dest,src,count);

	return dest;
}
