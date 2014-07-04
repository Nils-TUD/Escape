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

void *memcpy(void *dest,const void *src,size_t len) {
	uchar *bdest = (uchar*)dest;
	uchar *bsrc = (uchar*)src;
	/* copy bytes for alignment */
	if(((uintptr_t)bdest % sizeof(ulong)) == ((uintptr_t)bsrc % sizeof(ulong))) {
		while(len > 0 && (uintptr_t)bdest % sizeof(ulong)) {
			*bdest++ = *bsrc++;
			len--;
		}
	}

	ulong *ddest = (ulong*)bdest;
	ulong *dsrc = (ulong*)bsrc;
	/* copy words with loop-unrolling */
	while(len >= sizeof(ulong) * 16) {
		*ddest = *dsrc;
		*(ddest + 1) = *(dsrc + 1);
		*(ddest + 2) = *(dsrc + 2);
		*(ddest + 3) = *(dsrc + 3);
		*(ddest + 4) = *(dsrc + 4);
		*(ddest + 5) = *(dsrc + 5);
		*(ddest + 6) = *(dsrc + 6);
		*(ddest + 7) = *(dsrc + 7);
		*(ddest + 8) = *(dsrc + 8);
		*(ddest + 9) = *(dsrc + 9);
		*(ddest + 10) = *(dsrc + 10);
		*(ddest + 11) = *(dsrc + 11);
		*(ddest + 12) = *(dsrc + 12);
		*(ddest + 13) = *(dsrc + 13);
		*(ddest + 14) = *(dsrc + 14);
		*(ddest + 15) = *(dsrc + 15);
		ddest += 16;
		dsrc += 16;
		len -= sizeof(ulong) * 16;
	}

	/* copy remaining words */
	while(len >= sizeof(ulong)) {
		*ddest++ = *dsrc++;
		len -= sizeof(ulong);
	}

	/* copy remaining bytes */
	bdest = (uchar*)ddest;
	bsrc = (uchar*)dsrc;
	while(len-- > 0)
		*bdest++ = *bsrc++;
	return dest;
}
