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

#include <mem/pagedir.h>
#include <mem/physmem.h>
#include <boot.h>
#include <common.h>
#include <log.h>
#include <string.h>
#include <util.h>
#include <video.h>

bool PhysMem::canMap(uintptr_t addr,size_t size) {
	const Boot::Info *info = Boot::getInfo();
	/* go through the memory-map; if it overlaps with one of the free areas, its not allowed */
	for(size_t i = 0; i < info->mmapCount; ++i) {
		if(info->mmap[i].type == Boot::MemMap::MEM_AVAILABLE) {
			if(OVERLAPS(addr,addr + size,info->mmap[i].baseAddr,
					info->mmap[i].baseAddr + info->mmap[i].size))
				return false;
		}
	}
	return true;
}
