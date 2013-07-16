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

#include <sys/common.h>
#include <sys/mem/pmem.h>
#include <sys/mem/paging.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <sys/video.h>
#include <sys/log.h>
#include <string.h>

bool pmem_canMap(uintptr_t addr,size_t size) {
	sMemMap *mmap;
	const sBootInfo *mb = boot_getInfo();
	if(mb->mmapAddr == NULL)
		return false;
	/* go through the memory-map; if it overlaps with one of the free areas, its not allowed */
	for(mmap = mb->mmapAddr; (uintptr_t)mmap < (uintptr_t)mb->mmapAddr + mb->mmapLength;
			mmap = (sMemMap*)((uintptr_t)mmap + mmap->size + sizeof(mmap->size))) {
		if(mmap->type == MMAP_TYPE_AVAILABLE) {
			if(OVERLAPS(addr,addr + size,mmap->baseAddr,mmap->baseAddr + mmap->length))
				return false;
		}
	}
	return true;
}
