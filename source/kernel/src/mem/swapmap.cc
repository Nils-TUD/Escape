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
#include <sys/spinlock.h>
#include <sys/video.h>
#include <assert.h>

/* we need to maintain a reference-count here, because if we clone a process and a not-shareable
 * region contains swapped out pages, multiple regions will a share block. because we have to give
 * the child-process the content at that point of time, i.e. we can't demand-load it from disk in
 * this case. that means, both regions have to load this page from the swap-block. */

size_t SwapMap::totalBlocks = 0;
size_t SwapMap::freeBlocks = 0;
SwapMap::Block *SwapMap::swapBlocks = NULL;
SwapMap::Block *SwapMap::freeList = NULL;
klock_t SwapMap::lock;

bool SwapMap::init(size_t swapSize) {
	size_t i;
	totalBlocks = swapSize / PAGE_SIZE;
	freeBlocks = totalBlocks;
	swapBlocks = (Block*)Cache::alloc(totalBlocks * sizeof(Block));
	if(swapBlocks == NULL)
		return false;

	/* build freelist */
	freeList = swapBlocks + 0;
	swapBlocks[0].refCount = 0;
	swapBlocks[0].next = NULL;
	for(i = 1; i < totalBlocks; i++) {
		swapBlocks[i].next = freeList;
		swapBlocks[i].refCount = 0;
		freeList = swapBlocks + i;
	}
	return true;
}

ulong SwapMap::alloc() {
	Block *block;
	SpinLock::aquire(&lock);
	if(!freeList) {
		SpinLock::release(&lock);
		return INVALID_BLOCK;
	}
	block = freeList;
	freeList = freeList->next;
	block->refCount = 1;
	freeBlocks--;
	SpinLock::release(&lock);
	return block - swapBlocks;
}

void SwapMap::free(ulong block) {
	SpinLock::aquire(&lock);
	assert(block < totalBlocks);
	if(--swapBlocks[block].refCount == 0) {
		swapBlocks[block].next = freeList;
		freeList = swapBlocks + block;
		freeBlocks++;
	}
	SpinLock::release(&lock);
}

void SwapMap::print() {
	size_t i,c = 0;
	Video::printf("Size: %zu blocks (%zu KiB)\n",totalBlocks,(totalBlocks * PAGE_SIZE) / K);
	Video::printf("Free: %zu blocks (%zu KiB)\n",freeBlocks,(freeBlocks * PAGE_SIZE) / K);
	Video::printf("Used:");
	for(i = 0; i < totalBlocks; i++) {
		Block *block = swapBlocks + i;
		if(block->refCount > 0) {
			if(c % 8 == 0)
				Video::printf("\n ");
			Video::printf("%5u[%u] ",i,block->refCount);
			c++;
		}
	}
	Video::printf("\n");
}
