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

uintptr_t bitmapStart;

void pmem_initArch(uintptr_t *stackBegin,size_t *stackSize,tBitmap **bitmap) {
	size_t memSize,defPageCount;

	/* put the MM-stack behind the multiboot-pagetables */
	*stackBegin = boot_getMMStackBegin();

	/* calculate mm-stack-size */
	memSize = boot_getUsableMemCount();
	defPageCount = (memSize / PAGE_SIZE) - (BITMAP_PAGE_COUNT / PAGE_SIZE);
	*stackSize = (defPageCount + (PAGE_SIZE - 1) / sizeof(frameno_t)) / (PAGE_SIZE / sizeof(frameno_t));

	/* the first usable frame in the bitmap is behind the mm-stack */
	*bitmap = (tBitmap*)(*stackBegin + (*stackSize + 1) * PAGE_SIZE);
	*bitmap = (tBitmap*)(((uintptr_t)*bitmap + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));

	/* map that area */
	uintptr_t phys = boot_getModulesEnd() + BOOTSTRAP_PTS * PAGE_SIZE;
	phys = (phys + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

	uintptr_t virt = *stackBegin,end = (uintptr_t)*bitmap + BITMAP_PAGE_COUNT / 8;
	bitmapStart = phys + (*stackSize + 1) * PAGE_SIZE;
	while(virt < end) {
		paging_map(virt,&phys,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR | PG_ADDR_TO_FRAME);
		virt += PAGE_SIZE;
		phys += PAGE_SIZE;
	}

	/* mark all free */
	memset(*bitmap,0xFF,BITMAP_PAGE_COUNT / 8);
}

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

void pmem_markAvailable(void) {
	sMemMap *mmap;
	const sBootInfo *mb = boot_getInfo();
	/* now walk through the memory-map and mark all free areas as free */
	for(mmap = mb->mmapAddr; (uintptr_t)mmap < (uintptr_t)mb->mmapAddr + mb->mmapLength;
			mmap = (sMemMap*)((uintptr_t)mmap + mmap->size + sizeof(mmap->size))) {
		if(mmap != NULL && mmap->type == MMAP_TYPE_AVAILABLE) {
			/* take care that we don't use memory above 4G */
			if(mmap->baseAddr >= 0x100000000ULL) {
				log_printf("Skipping memory above 4G: %#Lx .. %#Lx\n",
						mmap->baseAddr,mmap->baseAddr + mmap->length);
				continue;
			}
			uint64_t end = mmap->baseAddr + mmap->length;
			if(end >= 0x100000000ULL) {
				log_printf("Skipping memory above 4G: %#Lx .. %#Lx\n",0x100000000ULL,end);
				end = 0xFFFFFFFF;
			}
			pmem_markRangeUsed(mmap->baseAddr,end,false);
		}
	}

	/* mark the bitmap used in itself */
	pmem_markRangeUsed(bitmapStart,bitmapStart + BITMAP_PAGE_COUNT / 8,true);
}
