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
#include <esc/debug.h>
#include <esc/thread.h>
#include <stdlib.h>
#include <assert.h>
#include "blockcache.h"

#define ALLOC_LOCK	0xF7180000

/* for statistics */
static u32 cacheHits = 0;
static u32 cacheMisses = 0;

/**
 * Aquires the lock, depending on <mode>, for the given block
 */
static void bcache_aquire(sCBlock *b,u8 mode);
/**
 * Releases the lock for given block
 */
static void bcache_doRelease(sCBlock *b,bool unlockAlloc);
/**
 * Requests the given block and reads it from disk if desired
 */
static sCBlock *bcache_doRequest(sBlockCache *c,u32 blockNo,bool doRead,u8 mode);
/**
 * Fetches a block-cache-entry
 */
static sCBlock *bcache_getBlock(sBlockCache *c);

void bcache_init(sBlockCache *c) {
	u32 i;
	sCBlock *bentry;
	c->usedBlocks = NULL;
	c->freeBlocks = NULL;
	c->oldestBlock = NULL;
	c->blockCache = (sCBlock*)malloc(c->blockCacheSize * sizeof(sCBlock));
	vassert(c->blockCache != NULL,"Unable to alloc mem for blockcache");
	bentry = c->blockCache;
	for(i = 0; i < c->blockCacheSize; i++) {
		bentry->blockNo = 0;
		bentry->buffer = NULL;
		bentry->dirty = false;
		bentry->refs = 0;
		bentry->prev = (i < c->blockCacheSize - 1) ? bentry + 1 : NULL;
		bentry->next = c->freeBlocks;
		c->freeBlocks = bentry;
		bentry++;
	}
}

void bcache_destroy(sBlockCache *c) {
	c->usedBlocks = NULL;
	c->freeBlocks = NULL;
	c->oldestBlock = NULL;
	free(c->blockCache);
}

void bcache_flush(sBlockCache *c) {
	sCBlock *bentry = c->usedBlocks;
	while(bentry != NULL) {
		assert(lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
		if(bentry->dirty) {
			vassert(c->write != NULL,"Block %d dirty, but no write-function",bentry->blockNo);
			bcache_aquire(bentry,BMODE_READ);
			c->write(c->handle,bentry->buffer,bentry->blockNo,1);
			bentry->dirty = false;
			bcache_doRelease(bentry,false);
		}
		assert(unlock(ALLOC_LOCK) == 0);
		bentry = bentry->next;
	}
}

void bcache_markDirty(sCBlock *b) {
	/*vassert(b->writeRef > 0,"Block %d is read-only!",b->blockNo);*/
	b->dirty = true;
}

sCBlock *bcache_create(sBlockCache *c,u32 blockNo) {
	return bcache_doRequest(c,blockNo,false,BMODE_WRITE);
}

sCBlock *bcache_request(sBlockCache *c,u32 blockNo,u8 mode) {
	return bcache_doRequest(c,blockNo,true,mode);
}

static void bcache_aquire(sCBlock *b,u8 mode) {
	assert(!(mode & BMODE_WRITE) || b->refs == 0);
	b->refs++;
	assert(unlock(ALLOC_LOCK) == 0);
	assert(lock((u32)b,(mode & BMODE_WRITE) ? LOCK_EXCLUSIVE : 0) == 0);
}

static void bcache_doRelease(sCBlock *b,bool unlockAlloc) {
	assert(lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	assert(b->refs > 0);
	b->refs--;
	if(unlockAlloc)
		assert(unlock(ALLOC_LOCK) == 0);
	assert(unlock((u32)b) == 0);
}

void bcache_release(sCBlock *b) {
	bcache_doRelease(b,true);
}

static sCBlock *bcache_doRequest(sBlockCache *c,u32 blockNo,bool doRead,u8 mode) {
	sCBlock *block,*bentry;

	/* aquire lock for getting a block */
	assert(lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);

	/* search for the block. perhaps it's already in cache */
	bentry = c->usedBlocks;
	while(bentry != NULL) {
		if(bentry->blockNo == blockNo) {
			/* remove from list and put at the beginning of the usedlist because it was
			 * used most recently */
			if(bentry->prev != NULL) {
				/* update oldest */
				if(c->oldestBlock == bentry)
					c->oldestBlock = bentry->prev;
				/* remove */
				bentry->prev->next = bentry->next;
				if(bentry->next)
					bentry->next->prev = bentry->prev;
				/* put at the beginning */
				bentry->prev = NULL;
				bentry->next = c->usedBlocks;
				bentry->next->prev = bentry;
				c->usedBlocks = bentry;
			}
			bcache_aquire(bentry,mode);
			cacheHits++;
			return bentry;
		}
		bentry = bentry->next;
	}

	/* init cached block */
	block = bcache_getBlock(c);
	if(block->buffer == NULL) {
		block->buffer = (u8*)malloc(c->blockSize);
		if(block->buffer == NULL) {
			assert(unlock(ALLOC_LOCK) == 0);
			return NULL;
		}
	}
	block->blockNo = blockNo;
	block->dirty = false;
	block->refs = 0;

	/* now read from disk */
	if(doRead) {
		/* we need always a write-lock because we have to read the content into it */
		bcache_aquire(block,BMODE_WRITE);
		if(!c->read(c->handle,block->buffer,blockNo,1)) {
			block->blockNo = 0;
			bcache_doRelease(block,true);
			return NULL;
		}
		bcache_doRelease(block,false);
	}

	bcache_aquire(block,mode);
	cacheMisses++;
	return block;
}

static sCBlock *bcache_getBlock(sBlockCache *c) {
	sCBlock *block = c->freeBlocks;
	if(block != NULL) {
		/* remove from freelist and put in usedlist */
		c->freeBlocks = block->next;
		if(c->freeBlocks)
			c->freeBlocks->prev = NULL;
		/* block->prev is already NULL */
		block->next = c->usedBlocks;
		if(block->next)
			block->next->prev = block;
		c->usedBlocks = block;
		/* update oldest */
		if(c->oldestBlock == NULL)
			c->oldestBlock = block;
		return block;
	}

	/* take the oldest one */
	block = c->oldestBlock;
	assert(block->refs == 0);
	c->oldestBlock = block->prev;
	c->oldestBlock->next = NULL;
	/* put at beginning of usedlist */
	block->prev = NULL;
	block->next = c->usedBlocks;
	if(block->next)
		block->next->prev = block;
	c->usedBlocks = block;
	/* if it is dirty we have to write it first to disk */
	if(block->dirty) {
		vassert(c->write != NULL,"Block %d dirty, but no write-function",block->blockNo);
		bcache_aquire(block,BMODE_READ);
		c->write(c->handle,block->buffer,block->blockNo,1);
		bcache_doRelease(block,false);
	}
	return block;
}

#if DEBUGGING

void bcache_print(sBlockCache *c) {
	u32 i = 0;
	sCBlock *block;
	printf("Used blocks:\n\t");
	block = c->usedBlocks;
	while(block != NULL) {
		if(++i % 8 == 0)
			printf("\n\t");
		printf("%d ",block->blockNo);
		block = block->next;
	}
	printf("\n");
}

void bcache_printStats(void) {
	printf("[BlockCache] Hits: %d, Misses: %d; %d %%\n",cacheHits,cacheMisses,
			(u32)(100 / ((float)(cacheMisses + cacheHits) / cacheHits)));
}

#endif
