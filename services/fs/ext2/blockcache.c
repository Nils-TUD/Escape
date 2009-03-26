/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <heap.h>
#include "ext2.h"
#include "request.h"
#include "blockcache.h"

/* for statistics */
static u32 cacheHits = 0;
static u32 cacheMisses = 0;

void ext2_bcache_init(sExt2 *e) {
	u32 i;
	sBCacheEntry *bentry = e->blockCache;
	for(i = 0; i < BLOCK_CACHE_SIZE; i++) {
		bentry->blockNo = 0;
		bentry->buffer = NULL;
		bentry++;
	}
	/* when overwriting entries, start with the first one */
	e->blockCachePos = 0;
}

u8 *ext2_bcache_request(sExt2 *e,u32 blockNo) {
	sBCacheEntry *block;

	/* search for the block. perhaps it's already in cache */
	u32 i;
	sBCacheEntry *bfree = NULL;
	sBCacheEntry *bentry = e->blockCache;
	for(i = 0; i < BLOCK_CACHE_SIZE; i++) {
		if(bentry->blockNo == blockNo) {
			cacheHits++;
			return bentry->buffer;
		}
		if(bfree == NULL && bentry->buffer == NULL)
			bfree = bentry;
		bentry++;
	}

	/* determine slot to use */
	if(bfree != NULL)
		block = bfree;
	else {
		/* don't overwrite the same block all the time. walk through the cache */
		block = e->blockCache + e->blockCachePos;
		e->blockCachePos = (e->blockCachePos + 1) % BLOCK_CACHE_SIZE;
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
