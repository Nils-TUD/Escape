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
#include <mem/pmem.h>
#include <mem/paging.h>
#include <util.h>
#include <video.h>
#include <assert.h>
#include <string.h>

/**
 * Marks the given range as used or not used
 *
 * @param from the start-address
 * @param to the end-address
 * @param used wether the frame is used
 */
static void mm_markAddrRangeUsed(u32 from,u32 to,bool used);

/**
 * Marks the given frame-number as used or not used
 *
 * @param frame the frame-number
 * @param used wether the frame is used
 */
static void mm_markFrameUsed(u32 frame,bool used);

#if DEBUGGING

/**
 * Checks wether the given frame of given type is free
 *
 * @param type the type: MM_DEF or MM_DMA
 * @param frame the frame-number
 * @return true if it is free
 */
static bool mm_dbg_isFrameFree(eMemType type,u32 frame);

#endif

/* start-address of the kernel */
extern u32 KernelStart;

/* the bitmap for the frames of the lower 16MB
 * 0 = free, 1 = used
 */
static u32 l16mBitmap[L16M_PAGE_COUNT / 32];
static u32 l16mCache[L16M_CACHE_SIZE];
static u32 l16mCachePos = 0;
static u32 l16mSearchPos = 0;

/* We use a stack for the memory at 16MB .. MAX.
 * TODO Currently we don't free the frames for the stack
 */

/* the number of frames we need for our stack */
static u32 u16mStackFrameCount = 0;
static u32 u16mStackStart = 0;
static u32 *u16mStack = NULL;

#if DEBUG_FRAME_USAGE
typedef struct {
	bool free;
	sProc *p;
	u32 frame;
} sFrameUsage;

#define FRAME_USAGE_COUNT 2048

static u32 frameUsagePos = 0;
static sFrameUsage frameUsages[FRAME_USAGE_COUNT];
#endif

void mm_init(void) {
	sMemMap *mmap;

	/* init stack */
	u16mStackFrameCount = (U16M_PAGE_COUNT + (PAGE_SIZE - 1) / sizeof(u32)) / (PAGE_SIZE / sizeof(u32));

	/* put the MM-stack behind the last multiboot-module */
	if(mb->modsCount == 0)
		util_panic("No multiboot-modules found");
	u16mStack = (u32*)(mb->modsAddr[mb->modsCount - 1].modEnd);
	u16mStackStart = (u32)u16mStack;

	/* at first we mark all frames as used in the bitmap for 0..16M */
	memset(l16mBitmap,0xFF,L16M_PAGE_COUNT / 8);

	/* now walk through the memory-map and mark all free areas as free */
	for(mmap = mb->mmapAddr;
		(u32)mmap < (u32)mb->mmapAddr + mb->mmapLength;
		mmap = (sMemMap*)((u32)mmap + mmap->size + sizeof(mmap->size))) {
		if(mmap != NULL && mmap->type == MMAP_TYPE_AVAILABLE) {
			mm_markAddrRangeUsed(mmap->baseAddr,mmap->baseAddr + mmap->length,false);
		}
	}

	/* mark the kernel-code and data (including stack for free frames) as used */
	/* Note that we have to remove the 0xC0000000 since we want to work with physical addresses */
	/* TODO we can use a little bit more because the kernel does not use the last few frames */
	mm_markAddrRangeUsed((u32)&KernelStart & ~KERNEL_AREA_V_ADDR,
			(u32)((u16mStackStart & ~KERNEL_AREA_V_ADDR) + u16mStackFrameCount * PAGE_SIZE),true);
}

u32 mm_getStackSize(void) {
	return u16mStackFrameCount * PAGE_SIZE;
}

/* TODO may be we should store and manipulate the current number of free frames? */
u32 mm_getFreeFrmCount(u32 types) {
	u32 count = 0;

	vassert(types & (MM_DMA | MM_DEF),"types is empty");
	vassert(!(types & ~(MM_DMA | MM_DEF)),"types contains invalid bits");

	if(types & MM_DMA) {
		/* count < 16MB frames */
		u32 i;
		for(i = 0; i < L16M_PAGE_COUNT; i++) {
			if((l16mBitmap[i >> 5] & (1 << (i & 0x1f))) == 0)
				count++;
		}
	}
	if(types & MM_DEF) {
		/* count > 16MB frames */
		count += ((u32)u16mStack - u16mStackStart) / sizeof(u32*);
	}
	return count;
}

void mm_allocateFrames(eMemType type,u32 *frames,u32 count) {
	vassert(type == MM_DEF || type == MM_DMA,"Invalid type");
	vassert(frames != NULL,"frames == NULL");

	while(count-- > 0) {
		*(frames++) = mm_allocateFrame(type);
	}
}

u32 mm_allocateFrame(eMemType type) {
	u32 bmIndex;

	vassert(type == MM_DEF || type == MM_DMA,"Invalid type");

	/* TODO what do we need for DMA? */
	if(type == MM_DMA) {
		/* is there a frame in the cache? */
		if(l16mCachePos > 0) {
			mm_markFrameUsed(l16mCache[l16mCachePos - 1],true);
			return l16mCache[--l16mCachePos];
		}

		/* fill the cache */
		for(; l16mSearchPos < L16M_PAGE_COUNT; l16mSearchPos++) {
			bmIndex = l16mSearchPos >> 5;
			if((l16mBitmap[bmIndex] & (1 << (l16mSearchPos & 0x1f))) == 0) {
				l16mCache[l16mCachePos++] = l16mSearchPos;
				if(l16mCachePos >= L16M_CACHE_SIZE) {
					break;
				}
			}
		}

		/* no frame found? */
		if(l16mCachePos == 0) {
			util_panic("Not enough memory :(");
		}

		mm_markFrameUsed(l16mCache[l16mCachePos - 1],true);
		return l16mCache[--l16mCachePos];
	}
	else {
		/* no more frames free? */
		if((u32)u16mStack == u16mStackStart) {
			util_panic("Not enough memory :(");
		}

#if DEBUG_FRAME_USAGE
		frameUsages[frameUsagePos].frame = *(u16mStack - 1);
		frameUsages[frameUsagePos].p = proc_getRunning();
		frameUsages[frameUsagePos].free = false;
		frameUsagePos++;
		vid_printf("a: %x of %d\n",*(u16mStack - 1),proc_getRunning()->pid);
		util_printStackTrace(util_getKernelStackTrace());
#endif
		return *(--u16mStack);
	}

	return 0;
}

void mm_freeFrames(eMemType type,u32 *frames,u32 count) {
	vassert(type == MM_DEF || type == MM_DMA,"Invalid type");
	vassert(frames != NULL,"frames == NULL");

	while(count-- > 0) {
		mm_freeFrame(*(frames++),type);
	}
}

void mm_freeFrame(u32 frame,eMemType type) {
	u32 *bitmapEntry;

	/*vassert(!mm_dbg_isFrameFree(type,frame),"Frame 0x%x (type %d) is already free",frame,type);*/

	/* TODO what do we need for DMA? */
	if(type == MM_DMA) {
		bitmapEntry = (u32*)(l16mBitmap + (frame >> 5));
		*bitmapEntry = *bitmapEntry & ~(1 << (frame & 0x1f));
		/* remember free frame in cache */
		if(l16mCachePos < L16M_CACHE_SIZE) {
			l16mCache[l16mCachePos++] = frame;
		}
	}
	else {
#if DEBUG_FRAME_USAGE
		u32 i;
		for(i = 0;i < FRAME_USAGE_COUNT;i++) {
			if(frameUsages[i].frame == frame && !frameUsages[i].free) {
				frameUsages[i].free = true;
				break;
			}
		}
		vid_printf("f: %x\n",frame);
#endif
		*(u16mStack++) = frame;
	}
}

static void mm_markAddrRangeUsed(u32 from,u32 to,bool used) {
	/* ensure that we start at a page-start */
	from &= ~PAGE_SIZE;
	for(; from < to; from += PAGE_SIZE) {
		mm_markFrameUsed(from >> PAGE_SIZE_SHIFT,used);
	}
}

static void mm_markFrameUsed(u32 frame,bool used) {
	u32 *bitmapEntry;
	/* we use a bitmap for the lower 16MB */
	if(frame < L16M_PAGE_COUNT) {
		bitmapEntry = (u32*)(l16mBitmap + (frame >> 5));
		if(used) {
			*bitmapEntry = *bitmapEntry | 1 << (frame & 0x1f);
		}
		else {
			*bitmapEntry = *bitmapEntry & ~(1 << (frame & 0x1f));
			/* remember free frame in cache */
			if(l16mCachePos < L16M_CACHE_SIZE) {
				l16mCache[l16mCachePos++] = frame;
			}
		}
	}
	/* use a stack for the remaining memory */
	else {
		/* we don't mark frames as used since this function is just used for initializing the
		 * memory-management */
		if(!used) {
			if((u32)u16mStack >= KERNEL_HEAP_START)
				util_panic("MM-Stack too small for physical memory!");
			/* Note that we assume that the kernel is not in the lower 16MB! */
			*u16mStack = frame;
			u16mStack++;
		}
	}
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

#if DEBUG_FRAME_USAGE
void mm_dbg_printFrameUsage(void) {
	u32 i;
	vid_printf("Currently used frames:\n");
	for(i = 0; i < FRAME_USAGE_COUNT; i++) {
		if(frameUsages[i].free == false && frameUsages[i].p)
			vid_printf("\tframe %x used by %d\n",frameUsages[i].frame,frameUsages[i].p->pid);
	}
}
#endif

static bool mm_dbg_isFrameFree(eMemType type,u32 frame) {
	u32 *ptr;
	if(type == MM_DEF) {
		ptr = u16mStack - 1;
		while((u32)ptr > u16mStackStart) {
			if(*ptr == frame)
				return true;
			ptr--;
		}
		return false;
	}

	ptr = (u32*)(l16mBitmap + (frame >> 5));
	return (*ptr & (1 << (frame & 0x1f))) == 0;
}

void mm_dbg_printFreeFrames(void) {
	u32 i,pos = 0;
	u32 *ptr;
	for(i = 0; i < ARRAY_SIZE(l16mBitmap); i++) {
		if(l16mBitmap[i] != ~0u)
			vid_printf("%08x..%08x: %032b\n",pos,pos + PAGE_SIZE * 32 - 1,l16mBitmap[i]);
		pos += PAGE_SIZE * 32;
	}

	vid_printf("Free frames cache:\n");
	for(i = 0;i < l16mCachePos; i++) {
		vid_printf("0x%08x, ",l16mCache[i]);
		if(i % 4 == 3)
			vid_printf("\n");
	}

	vid_printf("Free frames > 16MB:\n");
	for(i = 0,ptr = u16mStack - 1;(u32)ptr >= u16mStackStart; i++,ptr--) {
		vid_printf("0x%08x, ",*ptr);
		if(i % 4 == 3)
			vid_printf("\n");
	}
}

#endif
