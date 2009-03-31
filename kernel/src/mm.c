/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <mm.h>
#include <util.h>
#include <paging.h>
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

/**
 * Checks wether the given frame of given type is free
 *
 * @param type the type: MM_DEF or MM_DMA
 * @param frame the frame-number
 * @return true if it is free
 */
static bool mm_isFrameFree(eMemType type,u32 frame);

/* start- and end-address of the kernel */
extern u32 KernelStart;
extern u32 KernelEnd;

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
static u32 *u16mStack = NULL;

void mm_init(void) {
	sMemMap *mmap;

	/* init stack */
	u16mStackFrameCount = (U16M_PAGE_COUNT + (PAGE_SIZE - 1) / sizeof(u32)) / (PAGE_SIZE / sizeof(u32));
	/*vid_printf("MEMSIZE=%d bytes, PAGE_COUNT=%d, stackFrameCount=%d\n",
			MEMSIZE,U16M_PAGE_COUNT,u16mStackFrameCount);*/

	/* put the MM-stack behind the last multiboot-module */
	u16mStack = (u32*)mb->modsAddr[mb->modsCount - 1].modEnd;

	/* at first we mark all frames as used in the bitmap for 0..16M */
	memset(l16mBitmap,0xFFFFFFFF,L16M_PAGE_COUNT / 8);

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
			(u32)(((u32)&KernelEnd & ~KERNEL_AREA_V_ADDR) + u16mStackFrameCount * PAGE_SIZE),true);
}

/* TODO may be we should store and manipulate the current number of free frames? */
u32 mm_getFreeFrmCount(u32 types) {
	u32 i,bmIndex,count = 0;

	vassert(types & (MM_DMA | MM_DEF),"types is empty");
	vassert(!(types & ~(MM_DMA | MM_DEF)),"types contains invalid bits");

	if(types & MM_DMA) {
		/* count < 16MB frames */
		for(i = 0; i < L16M_PAGE_COUNT; i++) {
			bmIndex = l16mSearchPos >> 5;
			if((l16mBitmap[bmIndex] & (1 << (l16mSearchPos & 0x1f))) == 0) {
				count++;
			}
		}
	}
	if(types & MM_DEF) {
		/* count > 16MB frames */
		count += ((u32)u16mStack - (u32)&KernelEnd) / sizeof(u32*);
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
			panic("Not enough memory :(");
		}

		mm_markFrameUsed(l16mCache[l16mCachePos - 1],true);
		return l16mCache[--l16mCachePos];
	}
	else {
		/* no more frames free? */
		if((u32)u16mStack == (u32)&KernelEnd) {
			panic("Not enough memory :(");
		}

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

	/*vassert(!mm_isFrameFree(type,frame),"Frame 0x%x (type %d) is already free",frame,type);*/

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
		*(u16mStack++) = frame;
	}
}

static bool mm_isFrameFree(eMemType type,u32 frame) {
	u32 *ptr;
	if(type == MM_DEF) {
		ptr = u16mStack - 1;
		while((u32)ptr > (u32)&KernelEnd) {
			if(*ptr == frame)
				return true;
			ptr--;
		}
		return false;
	}

	ptr = (u32*)(l16mBitmap + (frame >> 5));
	return (*ptr & (1 << (frame & 0x1f))) == 0;
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
			/* Note that we assume that the kernel is not in the lower 16MB! */
			*u16mStack = frame;
			u16mStack++;
		}
	}
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void mm_dbg_printFreeFrames(void) {
	u32 i,pos = 0;
	u32 *ptr;
	for(i = 0; i < ARRAY_SIZE(l16mBitmap); i++) {
		if(l16mBitmap[i])
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
	for(i = 0,ptr = u16mStack - 1;ptr >= &KernelEnd; i++,ptr--) {
		vid_printf("0x%08x, ",*ptr);
		if(i % 4 == 3)
			vid_printf("\n");
	}
}

#endif
