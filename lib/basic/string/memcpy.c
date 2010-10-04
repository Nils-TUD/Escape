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

void *memcpy(void *dest,const void *src,size_t len) {
	uchar *bdest,*bsrc;
	/* copy dwords first with loop-unrolling */
	uint *ddest = (uint*)dest;
	uint *dsrc = (uint*)src;
	while(len >= sizeof(uint) * 8) {
		*ddest = *dsrc;
		*(ddest + 1) = *(dsrc + 1);
		*(ddest + 2) = *(dsrc + 2);
		*(ddest + 3) = *(dsrc + 3);
		*(ddest + 4) = *(dsrc + 4);
		*(ddest + 5) = *(dsrc + 5);
		*(ddest + 6) = *(dsrc + 6);
		*(ddest + 7) = *(dsrc + 7);
		ddest += 8;
		dsrc += 8;
		len -= sizeof(uint) * 8;
	}
	/* copy remaining dwords */
	while(len >= sizeof(uint)) {
		*ddest++ = *dsrc++;
		len -= sizeof(uint);
	}

	/* copy remaining bytes */
	bdest = (uchar*)ddest;
	bsrc = (uchar*)dsrc;
	while(len-- > 0)
		*bdest++ = *bsrc++;
	return dest;
}
