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

#include <esc/common.h>
#include <esc/heap.h>
#include <esc/debug.h>
#include "ext2.h"
#include "request.h"
#include "blockcache.h"

/* for statistics */
static u32 cacheHits = 0;
static u32 cacheMisses = 0;

/* note: it seems like that this approach is faster than using an array of linked-lists as hash-map.
 * Although we may have to search a bit more and may have "more chaos" in the array :)
 */

void ext2_bcache_init(sExt2 *e) {
	u32 i;
	sBCacheEntry *bentry = e->blockCache;
	for(i = 0; i < BLOCK_CACHE_SIZE; i++) {
		bentry->blockNo = 0;
		bentry->buffer = NULL;
		bentry++;
	}
	e->blockCacheFree = BLOCK_CACHE_SIZE;
}

u8 *ext2_bcache_request(sExt2 *e,u32 blockNo) {
	sBCacheEntry *block;

	/* search for the block. perhaps it's already in cache */
	/* note that we assume here that BLOCK_CACHE_SIZE is 2^x */
	sBCacheEntry *bentry;
	sBCacheEntry *bstart = e->blockCache + (blockNo & (BLOCK_CACHE_SIZE - 1));
	sBCacheEntry *bend = e->blockCache + BLOCK_CACHE_SIZE;
	for(bentry = bstart; bentry < bend; bentry++) {
		if(bentry->blockNo == blockNo) {
			cacheHits++;
			return bentry->buffer;
		}
	}
	/* search the entries before start */
	for(bentry = e->blockCache; bentry < bstart; bentry++) {
		if(bentry->blockNo == blockNo) {
			cacheHits++;
			return bentry->buffer;
		}
	}

	/* if there is a free block, try to find one beginning at the desired index */
	if(e->blockCacheFree > 0) {
		u32 no = blockNo;
		block = e->blockCache + (blockNo & (BLOCK_CACHE_SIZE - 1));
		/* the block should be free */
		while(block->buffer != NULL) {
			no = (no + 1) & (BLOCK_CACHE_SIZE - 1);
			block = e->blockCache + no;
		}
		e->blockCacheFree--;
	}
	else {
		/* no free blocks anymore so overwrite the one at our desired index */
		block = e->blockCache + (blockNo & (BLOCK_CACHE_SIZE - 1));
	}

	/* init cached block */
	if(block->buffer == NULL) {
		block->buffer = (u8*)malloc(BLOCK_SIZE(e));
		if(block->buffer == NULL)
			return NULL;
	}
	block->blockNo = blockNo;

	/* now read from disk */
	if(!ext2_readBlocks(e,block->buffer,blockNo,1)) {
		block->blockNo = 0;
		return NULL;
	}

	cacheMisses++;
	return block->buffer;
}

void ext2_bcache_printStats(void) {
	debugf("[BlockCache] Hits: %d, Misses: %d; %d %%\n",cacheHits,cacheMisses,
			(u32)(100 / ((float)(cacheMisses + cacheHits) / cacheHits)));
}
