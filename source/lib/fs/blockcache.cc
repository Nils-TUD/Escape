/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <fs/blockcache.h>
#include <fs/fsdev.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define ALLOC_LOCK	0xF7180000
#define HASH_SIZE	256

/**
 * Aquires the tpool_lock, depending on <mode>, for the given block
 */
static void bcache_acquire(sCBlock *b,uint mode);
/**
 * Releases the tpool_lock for given block
 */
static void bcache_doRelease(sCBlock *b,bool unlockAlloc);
/**
 * Requests the given block and reads it from disk if desired
 */
static sCBlock *bcache_doRequest(sBlockCache *c,block_t blockNo,bool doRead,uint mode);
/**
 * Fetches a block-cache-entry
 */
static sCBlock *bcache_getBlock(sBlockCache *c,block_t blockNo);

void bcache_init(sBlockCache *c,int fd) {
	size_t i;
	sCBlock *bentry;
	c->hashmap = (sCBlock**)calloc(HASH_SIZE,sizeof(sCBlock*));
	assert(c->hashmap != NULL);
	c->freeBlocks = NULL;
	c->oldestBlock = NULL;
	c->newestBlock = NULL;
	if(sharebuf(fd,c->blockCacheSize * c->blockSize,&c->blockmem,&c->blockshm,0) < 0) {
		if(c->blockmem == NULL)
			error("Unable to create block cache");
		printe("Unable to share buffer with disk driver");
	}
	c->blockCache = static_cast<sCBlock*>(malloc(c->blockCacheSize * sizeof(sCBlock)));
	assert(c->blockCache != NULL);
	bentry = c->blockCache;
	for(i = 0; i < c->blockCacheSize; i++) {
		bentry->blockNo = 0;
		bentry->buffer = (char*)c->blockmem + i * c->blockSize;
		bentry->dirty = false;
		bentry->refs = 0;
		bentry->prev = (i < c->blockCacheSize - 1) ? bentry + 1 : NULL;
		bentry->next = c->freeBlocks;
		bentry->hnext = NULL;
		c->freeBlocks = bentry;
		bentry++;
	}
	c->hits = 0;
	c->misses = 0;
}

void bcache_destroy(sBlockCache *c) {
	c->freeBlocks = NULL;
	c->oldestBlock = NULL;
	c->newestBlock = NULL;
	destroybuf(c->blockmem,c->blockshm);
	free(c->hashmap);
	free(c->blockCache);
}

void bcache_flush(sBlockCache *c) {
	sCBlock *bentry = c->newestBlock;
	while(bentry != NULL) {
		assert(tpool_lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
		if(bentry->dirty) {
			vassert(c->write != NULL,"Block %d dirty, but no write-function",bentry->blockNo);
			bcache_acquire(bentry,BMODE_READ);
			c->write(c->handle,bentry->buffer,bentry->blockNo,1);
			bentry->dirty = false;
			bcache_doRelease(bentry,false);
		}
		assert(tpool_unlock(ALLOC_LOCK) == 0);
		bentry = bentry->next;
	}
}

void bcache_markDirty(sCBlock *b) {
	/*vassert(b->writeRef > 0,"Block %d is read-only!",b->blockNo);*/
	b->dirty = true;
}

sCBlock *bcache_create(sBlockCache *c,block_t blockNo) {
	return bcache_doRequest(c,blockNo,false,BMODE_WRITE);
}

sCBlock *bcache_request(sBlockCache *c,block_t blockNo,uint mode) {
	return bcache_doRequest(c,blockNo,true,mode);
}

static void bcache_acquire(sCBlock *b,uint mode) {
	assert(!(mode & BMODE_WRITE) || b->refs == 0);
	b->refs++;
	assert(tpool_unlock(ALLOC_LOCK) == 0);
	assert(tpool_lock((uint)b,(mode & BMODE_WRITE) ? LOCK_EXCLUSIVE : 0) == 0);
}

static void bcache_doRelease(sCBlock *b,bool unlockAlloc) {
	assert(tpool_lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	assert(b->refs > 0);
	b->refs--;
	if(unlockAlloc)
		assert(tpool_unlock(ALLOC_LOCK) == 0);
	assert(tpool_unlock((uint)b) == 0);
}

void bcache_release(sCBlock *b) {
	bcache_doRelease(b,true);
}

static sCBlock *bcache_doRequest(sBlockCache *c,block_t blockNo,bool doRead,uint mode) {
	sCBlock *block,*bentry;

	/* acquire tpool_lock for getting a block */
	assert(tpool_lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);

	/* search for the block. perhaps it's already in cache */
	bentry = c->hashmap[blockNo % HASH_SIZE];
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
				bentry->next = c->newestBlock;
				bentry->next->prev = bentry;
				c->newestBlock = bentry;
			}
			bcache_acquire(bentry,mode);
			c->hits++;
			return bentry;
		}
		bentry = bentry->hnext;
	}

	/* init cached block */
	block = bcache_getBlock(c,blockNo);
	block->blockNo = blockNo;
	block->dirty = false;
	block->refs = 0;

	/* now read from disk */
	if(doRead) {
		/* we need always a write-tpool_lock because we have to read the content into it */
		bcache_acquire(block,BMODE_WRITE);
		if(c->read(c->handle,block->buffer,blockNo,1) != 0) {
			block->blockNo = 0;
			bcache_doRelease(block,true);
			return NULL;
		}
		bcache_doRelease(block,false);
	}

	bcache_acquire(block,mode);
	c->misses++;
	return block;
}

static sCBlock *bcache_getBlock(sBlockCache *c,block_t blockNo) {
	sCBlock *block = c->freeBlocks;
	if(block != NULL) {
		/* remove from freelist and put in usedlist */
		c->freeBlocks = block->next;
		if(c->freeBlocks)
			c->freeBlocks->prev = NULL;
		/* block->prev is already NULL */
		block->next = c->newestBlock;
		if(block->next)
			block->next->prev = block;
		c->newestBlock = block;
		/* update oldest */
		if(c->oldestBlock == NULL)
			c->oldestBlock = block;
		/* insert into hashmap */
		sCBlock **list = &c->hashmap[blockNo % HASH_SIZE];
		block->hnext = *list;
		*list = block;
		return block;
	}

	/* take the oldest one */
	block = c->oldestBlock;
	assert(block->refs == 0);
	c->oldestBlock = block->prev;
	c->oldestBlock->next = NULL;
	/* remove from hashmap */
	bool diffhash = block->blockNo % HASH_SIZE != blockNo % HASH_SIZE;
	if(diffhash) {
		sCBlock **list = &c->hashmap[block->blockNo % HASH_SIZE];
		sCBlock *b = *list, *p = NULL;
		while(b != NULL) {
			if(b == block) {
				if(p)
					p->hnext = b->hnext;
				else
					*list = b->hnext;
				break;
			}
			p = b;
			b = b->hnext;
		}
	}
	/* put at beginning of usedlist */
	block->prev = NULL;
	block->next = c->newestBlock;
	if(block->next)
		block->next->prev = block;
	c->newestBlock = block;
	/* insert into hashmap */
	if(diffhash) {
		sCBlock **list = &c->hashmap[blockNo % HASH_SIZE];
		block->hnext = *list;
		*list = block;
	}
	/* if it is dirty we have to write it first to disk */
	if(block->dirty) {
		vassert(c->write != NULL,"Block %d dirty, but no write-function",block->blockNo);
		bcache_acquire(block,BMODE_READ);
		c->write(c->handle,block->buffer,block->blockNo,1);
		bcache_doRelease(block,false);
	}
	return block;
}

void bcache_printStats(FILE *f,sBlockCache *c) {
	float hitrate;
	size_t used = 0,dirty = 0;
	sCBlock *bentry = c->newestBlock;
	while(bentry != NULL) {
		used++;
		if(bentry->dirty)
			dirty++;
		bentry = bentry->next;
	}
	fprintf(f,"\t\tTotal blocks: %zu\n",c->blockCacheSize);
	fprintf(f,"\t\tUsed blocks: %zu\n",used);
	fprintf(f,"\t\tDirty blocks: %zu\n",dirty);
	fprintf(f,"\t\tHits: %zu\n",c->hits);
	fprintf(f,"\t\tMisses: %zu\n",c->misses);
	if(c->hits == 0)
		hitrate = 0;
	else
		hitrate = 100.0f / ((float)(c->misses + c->hits) / c->hits);
	fprintf(f,"\t\tHitrate: %.3f%%\n",hitrate);
}

#if DEBUGGING

void bcache_print(sBlockCache *c) {
	size_t i = 0;
	sCBlock *block;
	printf("Used blocks:\n\t");
	block = c->newestBlock;
	while(block != NULL) {
		if(++i % 8 == 0)
			printf("\n\t");
		printf("%zu ",block->blockNo);
		block = block->next;
	}
	printf("\n");
}

#endif
