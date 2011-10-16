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
#include <sys/mem/swapmap.h>
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/klock.h>
#include <sys/video.h>
#include <assert.h>

/* we need to maintain a reference-count here, because if we clone a process and a not-shareable
 * region contains swapped out pages, multiple regions will a share block. because we have to give
 * the child-process the content at that point of time, i.e. we can't demand-load it from disk in
 * this case. that means, both regions have to load this page from the swap-block. */

typedef struct sSwapBlock {
	uint refCount;
	struct sSwapBlock *next;
} sSwapBlock;

static size_t totalBlocks = 0;
static size_t freeBlocks = 0;
static sSwapBlock *swapBlocks = NULL;
static sSwapBlock *freeList = NULL;
static klock_t swmapLock;

void swmap_init(size_t swapSize) {
	size_t i;
	totalBlocks = swapSize / PAGE_SIZE;
	freeBlocks = totalBlocks;
	swapBlocks = cache_alloc(totalBlocks * sizeof(sSwapBlock));
	if(swapBlocks == NULL)
		util_panic("Unable to allocate swap-blocks");

	/* build freelist */
	freeList = swapBlocks + 0;
	swapBlocks[0].refCount = 0;
	swapBlocks[0].next = NULL;
	for(i = 1; i < totalBlocks; i++) {
		swapBlocks[i].next = freeList;
		swapBlocks[i].refCount = 0;
		freeList = swapBlocks + i;
	}
}

ulong swmap_alloc(void) {
	sSwapBlock *block;
	klock_aquire(&swmapLock);
	if(!freeList) {
		klock_release(&swmapLock);
		return INVALID_BLOCK;
	}
	block = freeList;
	freeList = freeList->next;
	block->refCount = 1;
	freeBlocks--;
	klock_release(&swmapLock);
	return block - swapBlocks;
}

void swmap_incRefs(ulong block) {
	klock_aquire(&swmapLock);
	assert(block < totalBlocks && swapBlocks[block].refCount > 0);
	swapBlocks[block].refCount++;
	klock_release(&swmapLock);
}

bool swmap_isUsed(ulong block) {
	bool res;
	klock_aquire(&swmapLock);
	assert(block < totalBlocks);
	res = swapBlocks[block].refCount > 0;
	klock_release(&swmapLock);
	return res;
}

void swmap_free(ulong block) {
	klock_aquire(&swmapLock);
	assert(block < totalBlocks);
	if(--swapBlocks[block].refCount == 0) {
		swapBlocks[block].next = freeList;
		freeList = swapBlocks + block;
		freeBlocks++;
	}
	klock_release(&swmapLock);
}

size_t swmap_freeSpace(void) {
	return freeBlocks * PAGE_SIZE;
}

void swmap_print(void) {
	/* TODO sSwMapArea *a = used;
	vid_printf("SwapMap:\n");
	while(a != NULL) {
		if(!a->free) {
			sSLNode *n;
			vid_printf("\t%04u-%04u: ",a->block,a->block + a->size - 1);
			for(n = sll_begin(a->procs); n != NULL; n = n->next) {
				vid_printf("%d",((sProc*)n->data)->pid);
				if(n->next)
					vid_printf(",");
			}
			vid_printf(" (%p - %p)\n",a->virt,a->virt + a->size * PAGE_SIZE - 1);
		}
		else
			vid_printf("\t%04u-%04u: free\n",a->block,a->block + a->size - 1);
		a = a->next;
	}*/
}
