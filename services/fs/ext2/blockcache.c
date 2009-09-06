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
#include <assert.h>
#include "ext2.h"
#include "rw.h"
#include "blockcache.h"

/* for statistics */
static u32 cacheHits = 0;
static u32 cacheMisses = 0;

/**
 * Requests the given block and reads it from disk if desired
 */
static sExt2CBlock *ext2_bcache_doRequest(sExt2 *e,u32 blockNo,bool read);
/**
 * Fetches a block-cache-entry
 */
static sExt2CBlock *ext2_bcache_getBlock(sExt2 *e);

/* note: it seems like that this approach is faster than using an array of linked-lists as hash-map.
 * Although we may have to search a bit more and may have "more chaos" in the array :)
 */

void ext2_bcache_init(sExt2 *e) {
	u32 i;
	sExt2CBlock *bentry = e->blockCache;
	e->usedBlocks = NULL;
	e->freeBlocks = NULL;
	e->oldestBlock = NULL;
	for(i = 0; i < BLOCK_CACHE_SIZE; i++) {
		bentry->blockNo = 0;
		bentry->buffer = NULL;
		bentry->dirty = false;
		bentry->prev = i < BLOCK_CACHE_SIZE - 1 ? bentry + 1 : NULL;
		bentry->next = e->freeBlocks;
		e->freeBlocks = bentry;
		bentry++;
	}
}

void ext2_bcache_flush(sExt2 *e) {
	sExt2CBlock *bentry = e->usedBlocks;
	while(bentry != NULL) {
		if(bentry->dirty) {
			ext2_rw_writeBlocks(e,bentry->buffer,bentry->blockNo,1);
			bentry->dirty = false;
		}
		bentry = bentry->next;
	}
}

sExt2CBlock *ext2_bcache_create(sExt2 *e,u32 blockNo) {
	return ext2_bcache_doRequest(e,blockNo,false);
}

sExt2CBlock *ext2_bcache_request(sExt2 *e,u32 blockNo) {
	return ext2_bcache_doRequest(e,blockNo,true);
}

static sExt2CBlock *ext2_bcache_doRequest(sExt2 *e,u32 blockNo,bool read) {
	sExt2CBlock *block,*bentry;

	/* search for the block. perhaps it's already in cache */
	bentry = e->usedBlocks;
	while(bentry != NULL) {
		if(bentry->blockNo == blockNo) {
			/* remove from list and put at the beginning of the usedlist because it was
			 * used most recently */
			if(bentry->prev != NULL) {
				/* update oldest */
				if(e->oldestBlock == bentry)
					e->oldestBlock = bentry->prev;
				/* remove */
				bentry->prev->next = bentry->next;
				bentry->next->prev = bentry->prev;
				/* put at the beginning */
				bentry->prev = NULL;
				bentry->next = e->usedBlocks;
				bentry->next->prev = bentry;
				e->usedBlocks = bentry;
			}
			cacheHits++;
			return bentry;
		}
		bentry = bentry->next;
	}

	/* init cached block */
	block = ext2_bcache_getBlock(e);
	if(block->buffer == NULL) {
		block->buffer = (u8*)malloc(BLOCK_SIZE(e));
		if(block->buffer == NULL)
			return NULL;
	}
	block->blockNo = blockNo;
	block->dirty = false;

	/* now read from disk */
	if(read && !ext2_rw_readBlocks(e,block->buffer,blockNo,1)) {
		block->blockNo = 0;
		return NULL;
	}

	cacheMisses++;
	return block;
}

static sExt2CBlock *ext2_bcache_getBlock(sExt2 *e) {
	sExt2CBlock *block = e->freeBlocks;
	if(block != NULL) {
		/* remove from freelist and put in usedlist */
		e->freeBlocks = block->next;
		block->next->prev = NULL;
		/* block->prev is already NULL */
		block->next = e->usedBlocks;
		block->next->prev = block;
		e->usedBlocks = block;
		/* update oldest */
		if(e->oldestBlock == NULL)
			e->oldestBlock = block;
		return block;
	}

	/* take the oldest one */
	block = e->oldestBlock;
	e->oldestBlock = block->prev;
	e->oldestBlock->next = NULL;
	/* put at beginning of usedlist */
	block->prev = NULL;
	block->next = e->usedBlocks;
	block->next->prev = block;
	e->usedBlocks = block;
	/* if it is dirty we have to write it first to disk */
	if(block->dirty)
		ext2_rw_writeBlocks(e,block->buffer,block->blockNo,1);
	return block;
}

void ext2_bcache_print(sExt2 *e) {
	u32 i = 0;
	sExt2CBlock *block;
	debugf("Used blocks:\n\t");
	block = e->usedBlocks;
	while(block != NULL) {
		if(++i % 8 == 0)
			debugf("\n\t");
		debugf("%d ",block->blockNo);
		block = block->next;
	}
	debugf("\n");
}

void ext2_bcache_printStats(void) {
	debugf("[BlockCache] Hits: %d, Misses: %d; %d %%\n",cacheHits,cacheMisses,
			(u32)(100 / ((float)(cacheMisses + cacheHits) / cacheHits)));
}
