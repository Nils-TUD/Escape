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
#include <sys/video.h>
#include <assert.h>

/* The idea is:
 * We us a bitmap to store which block is free and which is used. Additionally we store the block-
 * number in which a page-content is in the region-flags. This way we simply look in the region-
 * flags to see in which block it is and load the content from there.
 * We don't swap out COW-pages, so that we know that a block is just used by exactly one region.
 * I.e. as soon as a region is removed all blocks in the swapmap are marked as free.
 */

static uint8_t *bitmap = NULL;
static size_t totalBlocks = 0;
static size_t freeBlocks = 0;
static ulong nextBlock = 0;

void swmap_init(size_t swapSize) {
	totalBlocks = swapSize / PAGE_SIZE;
	freeBlocks = totalBlocks;
	bitmap = cache_calloc(totalBlocks / 8,1);
	if(bitmap == NULL)
		util_panic("Unable to allocate swap-bitmap");
}

ulong swmap_alloc(void) {
	ulong i;
begin:
	for(i = nextBlock; i < totalBlocks; ) {
		size_t idx = i / 8;
		/* skip this byte if its completly used */
		if(bitmap[idx] == 0xFF)
			i += 8 - (i % 8);
		else {
			uint32_t bit = 1 << (i % 8);
			if(!(bitmap[idx] & bit)) {
				/* mark used */
				bitmap[idx] |= bit;
				freeBlocks--;
				nextBlock = i + 1;
				return i;
			}
			i++;
		}
	}
	if(nextBlock != 0) {
		nextBlock = 0;
		goto begin;
	}
	return INVALID_BLOCK;
}

bool swmap_isUsed(ulong block) {
	assert(block < totalBlocks);
	return bitmap[block / 8] & (1 << (block % 8));
}

void swmap_free(ulong block) {
	assert(block < totalBlocks);
	bitmap[block / 8] &= ~(1 << (block % 8));
	nextBlock = block;
	freeBlocks++;
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
