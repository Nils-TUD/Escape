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
#include <sys/klock.h>
#include <assert.h>
#include <string.h>
#include <errors.h>

#define DEBUG_ALLOC_N_FREE			0
#define BITS_PER_BMWORD				(sizeof(tBitmap) * 8)
#define BITMAP_START				((uintptr_t)bitmap - KERNEL_START)
#define BITMAP_START_FRAME			(BITMAP_START / PAGE_SIZE)

static void pmem_doMarkRangeUsed(uintptr_t from,uintptr_t to,bool used);
static void pmem_markUsed(frameno_t frame,bool used);

/* the bitmap for the frames of the lowest few MB
 * 0 = free, 1 = used
 */
static tBitmap *bitmap;
static size_t freeCont = 0;

/* We use a stack for the remaining memory
 * TODO Currently we don't free the frames for the stack
 */
static size_t stackSize = 0;
static uintptr_t stackBegin = 0;
static frameno_t *stack = NULL;
static klock_t pmemLock;

void pmem_init(void) {
	pmem_initArch(&stackBegin,&stackSize,&bitmap);
	stack = (frameno_t*)stackBegin;
	pmem_markAvailable();
}

size_t pmem_getStackSize(void) {
	return stackSize * PAGE_SIZE;
}

size_t pmem_getFreeFrames(uint types) {
	size_t count = 0;
	klock_aquire(&pmemLock);
	if(types & MM_CONT)
		count += freeCont;
	if(types & MM_DEF)
		count += ((uintptr_t)stack - stackBegin) / sizeof(frameno_t);
	klock_release(&pmemLock);
	return count;
}

ssize_t pmem_allocateContiguous(size_t count,size_t align) {
	size_t i,c = 0;
	klock_aquire(&pmemLock);
	/* align in physical memory */
	i = (BITMAP_START_FRAME + align - 1) & ~(align - 1);
	i -= BITMAP_START_FRAME;
	for(; i < BITMAP_PAGE_COUNT; ) {
		/* walk forward until we find an occupied frame */
		size_t j = i;
		for(c = 0; c < count; j++,c++) {
			tBitmap dword = bitmap[j / BITS_PER_BMWORD];
			tBitmap bit = (BITS_PER_BMWORD - 1) - (j % BITS_PER_BMWORD);
			if(dword & (1UL << bit))
				break;
		}
		/* found enough? */
		if(c == count)
			break;
		/* ok, to next aligned frame */
		i = (BITMAP_START_FRAME + j + 1 + align - 1) & ~(align - 1);
		i -= BITMAP_START_FRAME;
	}

	if(c != count) {
		klock_release(&pmemLock);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* the bitmap starts managing the memory at itself */
	i += BITMAP_START_FRAME;
	pmem_doMarkRangeUsed(i * PAGE_SIZE,(i + count) * PAGE_SIZE,true);
#if DEBUG_ALLOC_N_FREE
	vid_printf("[AC] %x:%zu\n",i,count);
#endif
	klock_release(&pmemLock);
	return i;
}

void pmem_freeContiguous(frameno_t first,size_t count) {
#if DEBUG_ALLOC_N_FREE
	vid_printf("[FC] %x:%zu\n",first,count);
#endif
	pmem_markRangeUsed(first * PAGE_SIZE,(first + count) * PAGE_SIZE,false);
}

frameno_t pmem_allocate(void) {
	frameno_t res;
	klock_aquire(&pmemLock);
	/* no more frames free? */
	if((uintptr_t)stack == stackBegin)
		util_panic("Not enough memory :(");

#if DEBUG_ALLOC_N_FREE
	sFuncCall *trace = util_getKernelStackTrace();
	size_t i = 0;
	vid_printf("[A] %x ",*(stack - 1));
	while(trace->addr != 0 && i++ < 10) {
		vid_printf("%x",trace->addr);
		trace++;
		if(trace->addr)
			vid_printf(" ");
	}
	vid_printf("\n");
#endif
	res = *(--stack);
	klock_release(&pmemLock);
	return res;
}

void pmem_free(frameno_t frame) {
	klock_aquire(&pmemLock);
#if DEBUG_ALLOC_N_FREE
	sFuncCall *trace = util_getKernelStackTrace();
	size_t i = 0;
	vid_printf("[F] %x ",frame);
	while(trace->addr != 0 && i++ < 10) {
		vid_printf("%x",trace->addr);
		trace++;
		if(trace->addr)
			vid_printf(" ");
	}
	vid_printf("\n");
#endif
	pmem_markUsed(frame,false);
	klock_release(&pmemLock);
}

void pmem_markRangeUsed(uintptr_t from,uintptr_t to,bool used) {
	klock_aquire(&pmemLock);
	pmem_doMarkRangeUsed(from,to,used);
	klock_release(&pmemLock);
}

void pmem_print(uint types) {
	size_t i,pos = BITMAP_START;
	frameno_t *ptr;
	if(types & MM_CONT) {
		vid_printf("Bitmap: (frame numbers)\n");
		for(i = 0; i < BITMAP_PAGE_COUNT / BITS_PER_BMWORD; i++) {
			vid_printf("0x%08Px..: %0*lb\n",pos / PAGE_SIZE,sizeof(tBitmap) * 8,bitmap[i]);
			pos += PAGE_SIZE * BITS_PER_BMWORD;
		}
	}
	if(types & MM_DEF) {
		vid_printf("Stack: (frame numbers)\n");
		for(i = 0,ptr = stack - 1;(uintptr_t)ptr >= stackBegin; i++,ptr--) {
			vid_printf("0x%08Px, ",*ptr);
			if(i % 4 == 3)
				vid_printf("\n");
		}
	}
}

static void pmem_doMarkRangeUsed(uintptr_t from,uintptr_t to,bool used) {
	/* ensure that we start at a page-start */
	from &= ~(PAGE_SIZE - 1);
	for(; from < to; from += PAGE_SIZE)
		pmem_markUsed(from >> PAGE_SIZE_SHIFT,used);
}

static void pmem_markUsed(frameno_t frame,bool used) {
	/* ignore the stuff before; we don't manage it */
	if(frame < BITMAP_START_FRAME)
		return;
	/* we use a bitmap for the lowest few MB */
	if(frame < BITMAP_START_FRAME + BITMAP_PAGE_COUNT) {
		tBitmap bit,*bitmapEntry;
		/* the bitmap starts managing the memory at itself */
		frame -= BITMAP_START_FRAME;
		bitmapEntry = (tBitmap*)(bitmap + (frame / BITS_PER_BMWORD));
		bit = (BITS_PER_BMWORD - 1) - (frame % BITS_PER_BMWORD);
		if(used) {
			*bitmapEntry = *bitmapEntry | (1UL << bit);
			freeCont--;
		}
		else {
			*bitmapEntry = *bitmapEntry & ~(1UL << bit);
			freeCont++;
		}
	}
	/* use a stack for the remaining memory */
	else {
		/* we don't mark frames as used since this function is just used for initializing the
		 * memory-management */
		if(!used) {
			if((uintptr_t)stack >= PMEM_END)
				util_panic("MM-Stack too small for physical memory!");
			*stack = frame;
			stack++;
		}
	}
}
