/**
 * $Id: cache.c 203 2011-05-12 08:42:18Z nasmussen $
 */

#include <assert.h>
#include <string.h>

#include "common.h"
#include "core/cache.h"
#include "core/cpu.h"
#include "core/register.h"
#include "core/mmu.h"
#include "core/bus.h"
#include "mmix/mem.h"
#include "config.h"
#include "event.h"

// fully associative, write-back, write-alloc cache, with random replacement policy and
// configureable block-size and block-count. no victim-cache here.

static void cache_invalidateBlock(sCache *c,sCacheBlock *block);
static sCacheBlock *cache_get(sCache *cache,octa addr,int flags,bool isWrite);
static sCacheBlock *cache_findVictim(const sCache *cache);
static void cache_fill(const sCache *cache,sCacheBlock *block,octa addr,int flags);
static void cache_flushBlock(const sCache *cache,sCacheBlock *block,int flags);

static octa nextVictim;
static sCache caches[] = {
	/* CACHE_INSTR */ {"ICache",ICACHE_BLOCK_NUM,ICACHE_BLOCK_SIZE,0,MSB(64),NULL,NULL},
	/* CACHE_DATA  */ {"DCache",DCACHE_BLOCK_NUM,DCACHE_BLOCK_SIZE,0,MSB(64),NULL,NULL},
};

void cache_init(void) {
	// create all caches
	nextVictim = 0;
	for(size_t i = 0; i < ARRAY_SIZE(caches); i++) {
		caches[i].tagMask = -(octa)caches[i].blockSize;
		caches[i].blocks = (sCacheBlock*)mem_alloc(sizeof(sCacheBlock) * caches[i].blockNum);
		for(size_t b = 0; b < caches[i].blockNum; b++) {
			sCacheBlock *block = caches[i].blocks + b;
			block->data = (octa*)mem_alloc(caches[i].blockSize);
			cache_invalidateBlock(caches + i,block);
		}
	}
}

void cache_reset(void) {
	cache_removeAll(CACHE_INSTR);
	cache_removeAll(CACHE_DATA);
	nextVictim = 0;
}

void cache_shutdown(void) {
	for(size_t i = 0; i < ARRAY_SIZE(caches); i++) {
		if(caches[i].blocks) {
			for(size_t b = 0; b < caches[i].blockNum; b++)
				mem_free(caches[i].blocks[b].data);
			mem_free(caches[i].blocks);
		}
	}
}

octa cache_read(int cache,octa addr,int flags) {
	assert(cache == CACHE_INSTR || cache == CACHE_DATA);
	// devices are uncached
	if(addr >= DEV_START_ADDR)
		return bus_read(addr,flags & MEM_SIDE_EFFECTS);

	sCache *c = caches + cache;
	sCacheBlock *block = cache_get(c,addr,flags,false);
	// the result is null if no side-effects or uncached is desired and we don't have it in cache
	if(!block)
		return bus_read(addr,flags & MEM_SIDE_EFFECTS);
	return block->data[(addr & ~c->tagMask) / sizeof(octa)];
}

void cache_write(int cache,octa addr,octa data,int flags) {
	assert(cache == CACHE_INSTR || cache == CACHE_DATA);
	// devices are uncached
	if(addr >= DEV_START_ADDR) {
		bus_write(addr,data,flags & MEM_SIDE_EFFECTS);
		return;
	}

	sCache *c = caches + cache;
	sCacheBlock *block = cache_get(c,addr,flags,true);
	// if it does exist in cache, write to cache
	if(block) {
		block->data[(addr & ~c->tagMask) / sizeof(octa)] = data;
		block->dirty = true;
	}
	// if not it means, uncached was desired: directly to memory
	else
		bus_write(addr,data,flags & MEM_SIDE_EFFECTS);
}

void cache_flush(int cache,octa addr) {
	assert(cache == CACHE_INSTR || cache == CACHE_DATA);
	sCacheBlock *block = (sCacheBlock*)cache_find(caches + cache,addr);
	if(block)
		cache_flushBlock(caches + cache,block,MEM_SIDE_EFFECTS);
}

void cache_flushAll(int cache) {
	assert(cache == CACHE_INSTR || cache == CACHE_DATA);
	sCache *c = caches + cache;
	for(size_t b = 0; b < c->blockNum; b++)
		cache_flushBlock(c,c->blocks + b,MEM_SIDE_EFFECTS);
}

void cache_remove(int cache,octa addr) {
	assert(cache == CACHE_INSTR || cache == CACHE_DATA);
	sCacheBlock *block = (sCacheBlock*)cache_find(caches + cache,addr);
	if(block)
		cache_invalidateBlock(caches + cache,block);
}

void cache_removeAll(int cache) {
	assert(cache == CACHE_INSTR || cache == CACHE_DATA);
	sCache *c = caches + cache;
	for(size_t b = 0; b < c->blockNum; b++) {
		sCacheBlock *block = c->blocks + b;
		cache_invalidateBlock(c,block);
	}
}

const sCache *cache_getCache(int cache) {
	assert(cache == CACHE_INSTR || cache == CACHE_DATA);
	return caches + cache;
}

bool cache_isValid(const sCacheBlock *block) {
	assert(block != NULL);
	return !(block->tag & MSB(64));
}

const sCacheBlock *cache_find(const sCache *cache,octa addr) {
	assert(cache != NULL);
	if(((cache->lastAddr ^ addr) & cache->tagMask) == 0)
		return cache->lastBlock;

	sCacheBlock *block = cache->blocks;
	sCacheBlock *end = block + cache->blockNum;
	octa tagMask = cache->tagMask;
	while(block < end) {
		if(((block->tag ^ addr) & tagMask) == 0) {
			sCache *wc = (sCache*)cache;
			wc->lastAddr = addr;
			wc->lastBlock = block;
			return block;
		}
		block++;
	}
	return NULL;
}

static void cache_invalidateBlock(sCache *c,sCacheBlock *block) {
	if(c->lastBlock == block) {
		c->lastAddr = MSB(64);
		c->lastBlock = NULL;
	}
	block->tag = MSB(64);
	block->dirty = false;
	// for testability
	memset(block->data,0,c->blockSize);
}

static sCacheBlock *cache_get(sCache *cache,octa addr,int flags,bool isWrite) {
	sCacheBlock *block = (sCacheBlock*)cache_find(cache,addr);
	if(flags & MEM_SIDE_EFFECTS)
		ev_fire2(EV_CACHE_LOOKUP,cache - caches,addr);

	if(block == NULL) {
		// if no sideeffects or uncached is desired, don't load it from memory; cached writes
		// will load it from memory in all cases (side-effects explicitly desired)
		if(!(flags & MEM_UNCACHED) && (isWrite || (flags & MEM_SIDE_EFFECTS))) {
			block = cache_findVictim(cache);
			if(block) {
				// write it back, if its dirty
				if(block->dirty)
					cache_flushBlock(cache,block,flags);
				if(!(flags & MEM_UNINITIALIZED))
					cache_fill(cache,block,addr,flags);
				else {
					block->tag = addr & cache->tagMask;
					memset(block->data,0,cache->blockSize);
				}
				if(cache->lastBlock == block)
					cache->lastAddr = MSB(64);
			}
		}
	}
	return block;
}

static sCacheBlock *cache_findVictim(const sCache *cache) {
	// first check if there is a free block
	for(size_t b = 0; b < cache->blockNum; b++) {
		sCacheBlock *block = cache->blocks + b;
		if(block->tag & MSB(64))
			return block;
	}
	// no free block, so pick a random victim
	if(cache->blockNum > 0)
		return cache->blocks + (nextVictim++ % cache->blockNum);
	return NULL;
}

static void cache_fill(const sCache *cache,sCacheBlock *block,octa addr,int flags) {
	// load the data for that block from mem
	size_t num = cache->blockSize / sizeof(octa);
	addr &= cache->tagMask;
	block->tag = addr;
	for(size_t i = 0; i < num; i++) {
		block->data[i] = bus_read(addr,flags & MEM_SIDE_EFFECTS);
		addr += sizeof(octa);
	}
	if(flags & MEM_SIDE_EFFECTS)
		ev_fire2(EV_CACHE_FILL,cache - caches,block->tag);
}

static void cache_flushBlock(const sCache *cache,sCacheBlock *block,int flags) {
	// nothing to do?
	if(!block->dirty)
		return;

	// write back to memory
	size_t num = cache->blockSize / sizeof(octa);
	octa addr = block->tag;
	for(size_t i = 0; i < num; i++) {
		bus_write(addr,block->data[i],flags & MEM_SIDE_EFFECTS);
		addr += sizeof(octa);
	}
	block->dirty = false;
	if(flags & MEM_SIDE_EFFECTS)
		ev_fire2(EV_CACHE_FLUSH,cache - caches,addr);
}
