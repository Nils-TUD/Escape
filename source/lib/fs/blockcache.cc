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

#include <fs/blockcache.h>
#include <fs/fsdev.h>
#include <sys/common.h>
#include <sys/debug.h>
#include <sys/thread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define ALLOC_LOCK	0xF7180000

namespace fs {

BlockCache::BlockCache(int fd,size_t blocks,size_t bsize)
		: _blockCacheSize(blocks), _blockSize(bsize), _hashmap(new CBlock*[HASH_SIZE]()),
		  _oldestBlock(NULL), _newestBlock(NULL), _freeBlocks(NULL),
		  _blockCache(new CBlock[blocks]), _blockmem(), _blockfd(), _hits(), _misses() {
	size_t i;
	CBlock *bentry;
	if((_blockfd = sharebuf(fd,_blockCacheSize * _blockSize,&_blockmem,0)) < 0) {
		if(_blockmem == NULL)
			VTHROW("Unable to create block cache");
		printe("Unable to share buffer with disk driver");
	}
	bentry = _blockCache;
	for(i = 0; i < _blockCacheSize; i++) {
		bentry->blockNo = 0;
		bentry->buffer = (char*)_blockmem + i * _blockSize;
		bentry->dirty = false;
		bentry->refs = 0;
		bentry->prev = (i < _blockCacheSize - 1) ? bentry + 1 : NULL;
		bentry->next = _freeBlocks;
		bentry->hnext = NULL;
		_freeBlocks = bentry;
		bentry++;
	}
}

BlockCache::~BlockCache() {
	destroybuf(_blockmem,_blockfd);
	delete[] _hashmap;
	delete[] _blockCache;
}

void BlockCache::flush() {
	CBlock *bentry = _newestBlock;
	while(bentry != NULL) {
		sassert(tpool_lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
		if(bentry->dirty) {
			acquire(bentry,READ);
			writeBlocks(bentry->buffer,bentry->blockNo,1);
			bentry->dirty = false;
			doRelease(bentry,false);
		}
		sassert(tpool_unlock(ALLOC_LOCK) == 0);
		bentry = bentry->next;
	}
}

void BlockCache::acquire(CBlock *b,A_UNUSED uint mode) {
	assert(!(mode & WRITE) || b->refs == 0);
	b->refs++;
	sassert(tpool_unlock(ALLOC_LOCK) == 0);
	sassert(tpool_lock((uint)b,(mode & WRITE) ? LOCK_EXCLUSIVE : 0) == 0);
}

void BlockCache::doRelease(CBlock *b,bool unlockAlloc) {
	sassert(tpool_lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	assert(b->refs > 0);
	b->refs--;
	if(unlockAlloc)
		sassert(tpool_unlock(ALLOC_LOCK) == 0);
	sassert(tpool_unlock((uint)b) == 0);
}

CBlock *BlockCache::doRequest(block_t blockNo,bool doRead,uint mode) {
	CBlock *block,*bentry;

	/* acquire tpool_lock for getting a block */
	sassert(tpool_lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);

	/* search for the block. perhaps it's already in cache */
	bentry = _hashmap[blockNo % HASH_SIZE];
	while(bentry != NULL) {
		if(bentry->blockNo == blockNo) {
			/* remove from list and put at the beginning of the usedlist because it was
			 * used most recently */
			if(bentry->prev != NULL) {
				/* update oldest */
				if(_oldestBlock == bentry)
					_oldestBlock = bentry->prev;
				/* remove */
				bentry->prev->next = bentry->next;
				if(bentry->next)
					bentry->next->prev = bentry->prev;
				/* put at the beginning */
				bentry->prev = NULL;
				bentry->next = _newestBlock;
				bentry->next->prev = bentry;
				_newestBlock = bentry;
			}
			acquire(bentry,mode);
			_hits++;
			return bentry;
		}
		bentry = bentry->hnext;
	}

	/* init cached block */
	block = getBlock(blockNo);
	block->blockNo = blockNo;
	block->dirty = false;
	block->refs = 0;

	/* now read from disk */
	if(doRead) {
		/* we need always a write-tpool_lock because we have to read the content into it */
		acquire(block,WRITE);
		if(readBlocks(block->buffer,blockNo,1) != 0) {
			block->blockNo = 0;
			doRelease(block,true);
			return NULL;
		}
		doRelease(block,false);
	}

	acquire(block,mode);
	_misses++;
	return block;
}

CBlock *BlockCache::getBlock(block_t blockNo) {
	CBlock *block = _freeBlocks;
	if(block != NULL) {
		/* remove from freelist and put in usedlist */
		_freeBlocks = block->next;
		if(_freeBlocks)
			_freeBlocks->prev = NULL;
		/* block->prev is already NULL */
		block->next = _newestBlock;
		if(block->next)
			block->next->prev = block;
		_newestBlock = block;
		/* update oldest */
		if(_oldestBlock == NULL)
			_oldestBlock = block;
		/* insert into hashmap */
		CBlock **list = &_hashmap[blockNo % HASH_SIZE];
		block->hnext = *list;
		*list = block;
		return block;
	}

	/* take the oldest one */
	block = _oldestBlock;
	assert(block->refs == 0);
	_oldestBlock = block->prev;
	_oldestBlock->next = NULL;
	/* remove from hashmap */
	bool diffhash = block->blockNo % HASH_SIZE != blockNo % HASH_SIZE;
	if(diffhash) {
		CBlock **list = &_hashmap[block->blockNo % HASH_SIZE];
		CBlock *b = *list, *p = NULL;
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
	block->next = _newestBlock;
	if(block->next)
		block->next->prev = block;
	_newestBlock = block;
	/* insert into hashmap */
	if(diffhash) {
		CBlock **list = &_hashmap[blockNo % HASH_SIZE];
		block->hnext = *list;
		*list = block;
	}
	/* if it is dirty we have to write it first to disk */
	if(block->dirty) {
		acquire(block,READ);
		writeBlocks(block->buffer,block->blockNo,1);
		doRelease(block,false);
	}
	return block;
}

void BlockCache::printStats(FILE *f) {
	float hitrate;
	size_t used = 0,dirty = 0;
	CBlock *bentry = _newestBlock;
	while(bentry != NULL) {
		used++;
		if(bentry->dirty)
			dirty++;
		bentry = bentry->next;
	}
	fprintf(f,"\t\tTotal blocks: %zu\n",_blockCacheSize);
	fprintf(f,"\t\tUsed blocks: %zu\n",used);
	fprintf(f,"\t\tDirty blocks: %zu\n",dirty);
	fprintf(f,"\t\tHits: %lu\n",_hits);
	fprintf(f,"\t\tMisses: %lu\n",_misses);
	if(_hits == 0)
		hitrate = 0;
	else
		hitrate = 100.0f / ((float)(_misses + _hits) / _hits);
	fprintf(f,"\t\tHitrate: %.3f%%\n",hitrate);
}

#if DEBUGGING

void BlockCache::print() {
	size_t i = 0;
	CBlock *block;
	printf("Used blocks:\n\t");
	block = _newestBlock;
	while(block != NULL) {
		if(++i % 8 == 0)
			printf("\n\t");
		printf("%zu ",block->blockNo);
		block = block->next;
	}
	printf("\n");
}

#endif

}
