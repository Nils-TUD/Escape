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

#include <sys/common.h>
#include <sys/mem/pmem.h>
#include <sys/mem/paging.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <sys/video.h>
#include <string.h>

static uintptr_t bitmapStart;

void pmem_initArch(uintptr_t *stackBegin,size_t *stackSize,tBitmap **bitmap) {
	size_t memSize,defPageCount;
	const sBootInfo *mb = boot_getInfo();

	/* put the MM-stack behind the last multiboot-module */
	if(mb->modsCount == 0)
		util_panic("No multiboot-modules found");
	*stackBegin = mb->modsAddr[mb->modsCount - 1].modEnd;

	/* calculate mm-stack-size */
	memSize = mb->memUpper * K;
	defPageCount = (memSize / PAGE_SIZE) - (BITMAP_PAGE_COUNT / PAGE_SIZE);
	*stackSize = (defPageCount + (PAGE_SIZE - 1) / sizeof(frameno_t)) / (PAGE_SIZE / sizeof(frameno_t));

	/* the first usable frame in the bitmap is behind the mm-stack */
	*bitmap = (tBitmap*)(*stackBegin + (*stackSize + 1) * PAGE_SIZE);
	*bitmap = (tBitmap*)(((uintptr_t)*bitmap + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
	bitmapStart = (uintptr_t)*bitmap - KERNEL_START;
	/* mark all free */
	memset(*bitmap,0xFF,BITMAP_PAGE_COUNT / 8);
}

void pmem_markAvailable(void) {
	sMemMap *mmap;
	const sBootInfo *mb = boot_getInfo();
	/* now walk through the memory-map and mark all free areas as free */
	for(mmap = mb->mmapAddr; (uintptr_t)mmap < (uintptr_t)mb->mmapAddr + mb->mmapLength;
			mmap = (sMemMap*)((uintptr_t)mmap + mmap->size + sizeof(mmap->size))) {
		if(mmap != NULL && mmap->type == MMAP_TYPE_AVAILABLE)
			pmem_markRangeUsed(mmap->baseAddr,mmap->baseAddr + mmap->length,false);
	}

	/* mark the bitmap used in itself */
	pmem_markRangeUsed(bitmapStart,bitmapStart + BITMAP_PAGE_COUNT / 8,true);
}
