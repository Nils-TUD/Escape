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

#include <common.h>
#include <proc.h>
#include <ioports.h>
#include <kheap.h>
#include <gdt.h>
#include <video.h>
#include <errors.h>
#include <string.h>

s32 ioports_request(sProc *p,u16 start,u16 count) {
	if(p->ioMap == NULL) {
		p->ioMap = (u8*)kheap_alloc(IO_MAP_SIZE / 8);
		if(p->ioMap == NULL)
			return ERR_NOT_ENOUGH_MEM;
		/* mark all as disallowed */
		memset(p->ioMap,0xFFFFFFFF,IO_MAP_SIZE / 8);
	}

	/* 0xF8 .. 0xFF is reserved */
	if(OVERLAPS(0xF8,0xFF + 1,start,start + count))
		return ERR_IO_MAP_RANGE_RESERVED;

	/* 0 means allowed */
	while(count-- > 0) {
		p->ioMap[start / 8] &= ~(1 << (start % 8));
		start++;
	}

	return 0;
}

s32 ioports_release(sProc *p,u16 start,u16 count) {
	if(p->ioMap == NULL)
		return ERR_IOMAP_NOT_PRESENT;

	/* 0xF8 .. 0xFF is reserved */
	if(OVERLAPS(0xF8,0xFF + 1,start,start + count))
		return ERR_IO_MAP_RANGE_RESERVED;

	/* 1 means disallowed */
	while(count-- > 0) {
		p->ioMap[start / 8] |= 1 << (start % 8);
		start++;
	}

	return 0;
}

/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void ioports_dbg_print(u8 *map) {
	u32 i,j,c = 0;
	vid_printf("Reserved IO-ports:\n\t");
	for(i = 0; i < IO_MAP_SIZE / 8; i++) {
		for(j = 0; j < 8; j++) {
			if(!(map[i] & (1 << j))) {
				vid_printf("%x, ",i * 8 + j);
				if(++c % 10 == 0)
					vid_printf("\n\t");
			}
		}
	}
}

#endif
