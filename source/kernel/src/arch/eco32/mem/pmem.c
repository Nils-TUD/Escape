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
#include <assert.h>
#include <string.h>
#include <errors.h>

#define BITMAP_PAGE_COUNT			((2 * M) / PAGE_SIZE)
#define BITMAP_START_FRAME			(BITMAP_START / PAGE_SIZE)
#define BITMAP_START				((uintptr_t)bitmap - DIR_MAPPED_SPACE)

static void pmem_markRangeUsed(uintptr_t from,uintptr_t to,bool used);
static void pmem_markUsed(tFrameNo frame,bool used);

/* start-address of the kernel */
extern uintptr_t KernelStart;

/* the bitmap for the frames of the lowest few MB
 * 0 = free, 1 = used
 */
static uint32_t *bitmap;
static size_t freeCont;

/* We use a stack for the remaining memory
 * TODO Currently we don't free the frames for the stack
 */

static size_t stackSize = 0;
static uintptr_t stackBegin = 0;
static tFrameNo *stack = NULL;

void pmem_init(void) {
	const sBootInfo *binfo = boot_getInfo();
	const sLoadProg *last;
	size_t defPageCount;

	/* put the mm-stack behind the last boot-module */
	if(binfo->progCount == 0)
		util_panic("No boot-modules found");
	last = binfo->progs + binfo->progCount - 1;
	/* word align the stack begin */
	stack = (tFrameNo*)((last->start + last->size + sizeof(int) - 1) & ~(sizeof(int) - 1));
	stackBegin = (uintptr_t)stack;

	/* calculate mm-stack-size */
	defPageCount = (binfo->memSize / PAGE_SIZE) - (BITMAP_PAGE_COUNT / PAGE_SIZE);
	stackSize = (defPageCount + (PAGE_SIZE - 1) / sizeof(tFrameNo)) / (PAGE_SIZE / sizeof(tFrameNo));

	/* the first usable frame in the bitmap is behind the mm-stack */
	bitmap = (uint32_t*)(stackBegin + (stackSize + 1) * PAGE_SIZE);
	bitmap = (uint32_t*)(((uintptr_t)bitmap + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
	/* mark all free */
	memset(bitmap,0xFF,BITMAP_PAGE_COUNT / 8);
	freeCont = 0;

	/* mark bitmap used */
	pmem_markRangeUsed(BITMAP_START,BITMAP_START + BYTES_2_PAGES(BITMAP_PAGE_COUNT / 8),true);
	/* mark other frames unused */
	pmem_markRangeUsed(BITMAP_START + BYTES_2_PAGES(BITMAP_PAGE_COUNT / 8),binfo->memSize,false);
}

size_t pmem_getStackSize(void) {
	return stackSize * PAGE_SIZE;
}

size_t pmem_getFreeFrames(uint types) {
	size_t count = 0;
	if(types & MM_CONT)
		count += freeCont;
	if(types & MM_DEF)
		count += ((uintptr_t)stack - stackBegin) / sizeof(tFrameNo);
	return count;
}

ssize_t pmem_allocateContiguous(size_t count,size_t align) {
	size_t i,c = 0;
	/* align in physical memory */
	i = (BITMAP_START_FRAME + align - 1) & ~(align - 1);
	i -= BITMAP_START_FRAME;
	for(; i < BITMAP_PAGE_COUNT; ) {
		/* walk forward until we find an occupied frame */
		size_t j = i;
		for(c = 0; c < count; j++,c++) {
			uint32_t dword = bitmap[j / 32];
			uint32_t bit = 31 - (j % 32);
			if(dword & (1 << bit))
				break;
		}
		/* found enough? */
		if(c == count)
			break;
		/* ok, to next aligned frame */
		i = (BITMAP_START_FRAME + j + 1 + align - 1) & ~(align - 1);
		i -= BITMAP_START_FRAME;
	}

	if(c != count)
		return ERR_NOT_ENOUGH_MEM;

	/* the bitmap starts managing the memory at itself */
	i += BITMAP_START_FRAME;
	pmem_markRangeUsed(i * PAGE_SIZE,(i + count) * PAGE_SIZE,true);
	return i;
}

void pmem_freeContiguous(tFrameNo first,size_t count) {
	pmem_markRangeUsed(first * PAGE_SIZE,(first + count) * PAGE_SIZE,false);
}

tFrameNo pmem_allocate(void) {
	/* no more frames free? */
	if((uintptr_t)stack == stackBegin)
		util_panic("Not enough memory :(");
	return *(--stack);
}

void pmem_free(tFrameNo frame) {
	pmem_markUsed(frame,false);
}

static void pmem_markRangeUsed(uintptr_t from,uintptr_t to,bool used) {
	/* ensure that we start at a page-start */
	from &= ~(PAGE_SIZE - 1);
	for(; from < to; from += PAGE_SIZE)
		pmem_markUsed(from >> PAGE_SIZE_SHIFT,used);
}

static void pmem_markUsed(tFrameNo frame,bool used) {
	/* ignore the stuff before; we don't manage it */
	if(frame < BITMAP_START_FRAME)
		return;
	/* we use a bitmap for the lowest few MB */
	if(frame < BITMAP_START_FRAME + BITMAP_PAGE_COUNT) {
		uint32_t bit,*bitmapEntry;
		/* the bitmap starts managing the memory at itself */
		frame -= BITMAP_START_FRAME;
		bitmapEntry = (uint32_t*)(bitmap + (frame / 32));
		bit = 31 - (frame % 32);
		if(used) {
			*bitmapEntry = *bitmapEntry | (1 << bit);
			freeCont--;
		}
		else {
			*bitmapEntry = *bitmapEntry & ~(1 << bit);
			freeCont++;
		}
	}
	/* use a stack for the remaining memory */
	else {
		/* we don't mark frames as used since this function is just used for initializing the
		 * memory-management */
		if(!used) {
			*stack = frame;
			stack++;
		}
	}
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void pmem_dbg_printFreeFrames(uint types) {
	size_t i,pos = BITMAP_START;
	tFrameNo *ptr;
	if(types & MM_CONT) {
		vid_printf("Bitmap:\n");
		for(i = 0; i < BITMAP_PAGE_COUNT / 8; i++) {
			vid_printf("%08Sx..%08Sx: %032b\n",pos,pos + PAGE_SIZE * 32 - 1,bitmap[i]);
			pos += PAGE_SIZE * 32;
		}
	}
	if(types & MM_DEF) {
		vid_printf("Stack:\n");
		for(i = 0,ptr = stack - 1;(uintptr_t)ptr >= stackBegin; i++,ptr--) {
			vid_printf("0x%08x, ",*ptr);
			if(i % 4 == 3)
				vid_printf("\n");
		}
	}
}

#endif
