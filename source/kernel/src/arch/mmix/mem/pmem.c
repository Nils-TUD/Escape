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
#include <assert.h>
#include <string.h>
#include <errno.h>

static uintptr_t bitmapStart;

void pmem_initArch(uintptr_t *stackBegin,size_t *stackSize,tBitmap **bitmap) {
	size_t defPageCount;
	const sBootInfo *binfo = boot_getInfo();
	const sLoadProg *last;

	/* put the mm-stack behind the last boot-module */
	if(binfo->progCount == 0)
		util_panic("No boot-modules found");
	last = binfo->progs + binfo->progCount - 1;
	/* word align the stack begin */
	*stackBegin = (last->start + last->size + sizeof(long) - 1) & ~(sizeof(long) - 1);

	/* calculate mm-stack-size */
	defPageCount = (binfo->memSize / PAGE_SIZE) - (BITMAP_PAGE_COUNT / PAGE_SIZE);
	*stackSize = (defPageCount + (PAGE_SIZE - 1) / sizeof(frameno_t)) / (PAGE_SIZE / sizeof(frameno_t));

	/* the first usable frame in the bitmap is behind the mm-stack */
	*bitmap = (tBitmap*)(*stackBegin + (*stackSize + 1) * PAGE_SIZE);
	*bitmap = (tBitmap*)(((uintptr_t)*bitmap + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
	bitmapStart = (uintptr_t)*bitmap & ~DIR_MAPPED_SPACE;
	/* mark all free */
	memset(*bitmap,0xFF,BITMAP_PAGE_COUNT / 8);
}

void pmem_markAvailable(void) {
	const sBootInfo *binfo = boot_getInfo();
	/* mark bitmap used */
	pmem_markRangeUsed(bitmapStart,bitmapStart + BITMAP_PAGE_COUNT / 8,true);
	/* mark other frames unused */
	pmem_markRangeUsed(bitmapStart + BITMAP_PAGE_COUNT / 8 + PAGE_SIZE - 1,binfo->memSize,false);
}
