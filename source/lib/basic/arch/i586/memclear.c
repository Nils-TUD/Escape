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

void memclear(void *addr,size_t count) {
	uchar *baddr;
	/* clear dwords first with loop-unrolling */
	uint *daddr = (uint*)addr;
	while(count >= sizeof(uint) * 8) {
		*daddr = 0;
		*(daddr + 1) = 0;
		*(daddr + 2) = 0;
		*(daddr + 3) = 0;
		*(daddr + 4) = 0;
		*(daddr + 5) = 0;
		*(daddr + 6) = 0;
		*(daddr + 7) = 0;
		daddr += 8;
		count -= sizeof(uint) * 8;
	}
	/* clear remaining dwords */
	while(count >= sizeof(uint)) {
		*daddr++ = 0;
		count -= sizeof(uint);
	}

	/* clear remaining bytes */
	baddr = (uchar*)daddr;
	while(count-- > 0)
		*baddr++ = 0;
}
