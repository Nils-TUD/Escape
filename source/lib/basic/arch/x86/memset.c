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

#if defined(KASAN)
void __asan_storeN_noabort(const void *addr,size_t size);
#endif

/* this is necessary to prevent that gcc transforms a loop into library-calls
 * (which might lead to recursion here) */
#pragma GCC optimize ("no-tree-loop-distribute-patterns")

A_NOASAN void *memset(void *addr,int value,size_t count) {
#if defined(KASAN)
	__asan_storeN_noabort(addr,count);
#endif

	uchar *baddr = (uchar*)addr;
	/* align it */
	while(count > 0 && (uintptr_t)baddr % sizeof(ulong)) {
		*baddr++ = value;
		count--;
	}

	ulong dwval = (value << 24) | (value << 16) | (value << 8) | value;
#if defined(__x86_64__)
	dwval |= ((ulong)value << 56) | ((ulong)value << 48);
	dwval |= ((ulong)value << 40) | ((ulong)value << 32);
#endif
	ulong *dwaddr = (ulong*)baddr;
	/* set words with loop-unrolling */
	while(count >= sizeof(ulong) * 16) {
		*dwaddr = dwval;
		*(dwaddr + 1) = dwval;
		*(dwaddr + 2) = dwval;
		*(dwaddr + 3) = dwval;
		*(dwaddr + 4) = dwval;
		*(dwaddr + 5) = dwval;
		*(dwaddr + 6) = dwval;
		*(dwaddr + 7) = dwval;
		*(dwaddr + 8) = dwval;
		*(dwaddr + 9) = dwval;
		*(dwaddr + 10) = dwval;
		*(dwaddr + 11) = dwval;
		*(dwaddr + 12) = dwval;
		*(dwaddr + 13) = dwval;
		*(dwaddr + 14) = dwval;
		*(dwaddr + 15) = dwval;
		dwaddr += 16;
		count -= sizeof(ulong) * 16;
	}

	/* set with dwords */
	while(count >= sizeof(ulong)) {
		*dwaddr++ = dwval;
		count -= sizeof(ulong);
	}

	/* set remaining bytes */
	baddr = (uchar*)dwaddr;
	while(count-- > 0)
		*baddr++ = value;
	return addr;
}
