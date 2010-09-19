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
#include <sys/util.h>
#include <sys/video.h>
#include <assert.h>
#include <string.h>
#include <errors.h>

#define BITMAP_PAGE_COUNT			((2 * M) / PAGE_SIZE)
#define BITMAP_START_FRAME			(BITMAP_START / PAGE_SIZE)
#define BITMAP_START				((u32)bitmap - KERNEL_AREA_V_ADDR)

/**
 * Marks the given range as used or not used
 *
 * @param from the start-address
 * @param to the end-address
 * @param used whether the frame is used
 */
static void mm_markRangeUsed(u32 from,u32 to,bool used);

/**
 * Marks the given frame-number as used or not used
 *
 * @param frame the frame-number
 * @param used whether the frame is used
 */
static void mm_markUsed(u32 frame,bool used);

/* start-address of the kernel */
extern u32 KernelStart;

/* the bitmap for the frames of the lowest few MB
 * 0 = free, 1 = used
 */
static u32 *bitmap;
static u32 freeCont;

/* We use a stack for the remaining memory
 * TODO Currently we don't free the frames for the stack
 */

static u32 stackSize = 0;
static u32 stackBegin = 0;
static u32 *stack = NULL;

void mm_init(const sMultiBoot *mb) {
	sMemMap *mmap;
	u32 memSize,defPageCount;

	/* put the MM-stack behind the last multiboot-module */
	if(mb->modsCount == 0)
		util_panic("No multiboot-modules found");
	stack = (u32*)(mb->modsAddr[mb->modsCount - 1].modEnd);
	stackBegin = (u32)stack;

	/* calculate mm-stack-size */
	memSize = mb->memUpper * K;
	defPageCount = (memSize / PAGE_SIZE) - (BITMAP_PAGE_COUNT / PAGE_SIZE);
	stackSize = (defPageCount + (PAGE_SIZE - 1) / sizeof(u32)) / (PAGE_SIZE / sizeof(u32));

	/* the first usable frame in the bitmap is behind the mm-stack */
	bitmap = (u32*)(stackBegin + (stackSize + 1) * PAGE_SIZE);
	bitmap = (u32*)(((u32)bitmap + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
	/* mark all free */
	memset(bitmap,0xFF,BITMAP_PAGE_COUNT / 8);
	freeCont = 0;

	/* now walk through the memory-map and mark all free areas as free */
	for(mmap = mb->mmapAddr; (u32)mmap < (u32)mb->mmapAddr + mb->mmapLength;
			mmap = (sMemMap*)((u32)mmap + mmap->size + sizeof(mmap->size))) {
		if(mmap != NULL && mmap->type == MMAP_TYPE_AVAILABLE)
			mm_markRangeUsed(mmap->baseAddr,mmap->baseAddr + mmap->length,false);
	}

	/* mark the bitmap used in itself */
	mm_markRangeUsed(BITMAP_START,BITMAP_START + BYTES_2_PAGES(BITMAP_PAGE_COUNT / 8),true);
}

u32 mm_getStackSize(void) {
	return stackSize * PAGE_SIZE;
}

u32 mm_getFreeFrames(u32 types) {
	u32 count = 0;
	if(types & MM_CONT)
		count += freeCont;
	if(types & MM_DEF)
		count += ((u32)stack - stackBegin) / sizeof(u32*);
	return count;
}

s32 mm_allocateContiguous(u32 count,u32 align) {
	u32 i,c = 0;
	/* align in physical memory */
	i = (BITMAP_START_FRAME + align - 1) & ~(align - 1);
	i -= BITMAP_START_FRAME;
	for(; i < BITMAP_PAGE_COUNT; ) {
		/* walk forward until we find an occupied frame */
		u32 j = i;
		for(c = 0; c < count; j++,c++) {
			u32 dword = bitmap[j / 32];
			u32 bit = 31 - (j % 32);
			if(dword & (1 << bit))
				break;
		}
		/* found enough? */
		if(c == count)
			break;
		/* ok, to next aligned frame */
		i = (j + 1 + align - 1) & ~(align - 1);
	}

	if(c != count)
		return ERR_NOT_ENOUGH_MEM;

	/* the bitmap starts managing the memory at itself */
	i += BITMAP_START_FRAME;
	mm_markRangeUsed(i * PAGE_SIZE,(i + count) * PAGE_SIZE,true);
	return i;
}

void mm_freeContiguous(u32 first,u32 count) {
	mm_markRangeUsed(first * PAGE_SIZE,(first + count) * PAGE_SIZE,false);
}

u32 mm_allocate(void) {
	/* no more frames free? */
	if((u32)stack == stackBegin)
		util_panic("Not enough memory :(");
	return *(--stack);
}

void mm_free(u32 frame) {
	mm_markUsed(frame,false);
}

static void mm_markRangeUsed(u32 from,u32 to,bool used) {
	/* ensure that we start at a page-start */
	from &= ~(PAGE_SIZE - 1);
	for(; from < to; from += PAGE_SIZE)
		mm_markUsed(from >> PAGE_SIZE_SHIFT,used);
}

static void mm_markUsed(u32 frame,bool used) {
	/* ignore the stuff before; we don't manage it */
	if(frame < BITMAP_START_FRAME)
		return;
	/* we use a bitmap for the lowest few MB */
	if(frame < BITMAP_START_FRAME + BITMAP_PAGE_COUNT) {
		u32 bit,*bitmapEntry;
		/* the bitmap starts managing the memory at itself */
		frame -= BITMAP_START_FRAME;
		bitmapEntry = (u32*)(bitmap + (frame / 32));
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
			if((u32)stack >= KERNEL_HEAP_START)
				util_panic("MM-Stack too small for physical memory!");
			*stack = frame;
			stack++;
		}
	}
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void mm_dbg_printFreeFrames(u32 types) {
	u32 i,pos = BITMAP_START;
	u32 *ptr;
	if(types & MM_CONT) {
		vid_printf("Bitmap:\n");
		for(i = 0; i < BITMAP_PAGE_COUNT / 8; i++) {
			vid_printf("%08x..%08x: %032b\n",pos,pos + PAGE_SIZE * 32 - 1,bitmap[i]);
			pos += PAGE_SIZE * 32;
		}
	}
	if(types & MM_DEF) {
		vid_printf("Stack:\n");
		for(i = 0,ptr = stack - 1;(u32)ptr >= stackBegin; i++,ptr--) {
			vid_printf("0x%08x, ",*ptr);
			if(i % 4 == 3)
				vid_printf("\n");
		}
	}
}

#endif
