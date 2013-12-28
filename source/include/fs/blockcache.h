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

#pragma once

#include <esc/common.h>
#include <stdio.h>

#define BMODE_READ		0x1
#define BMODE_WRITE		0x2

/* reading/writing of blocks */
typedef bool (*fReadBlocks)(void *h,void *buffer,block_t start,size_t blockCount);
typedef bool (*fWriteBlocks)(void *h,const void *buffer,size_t start,size_t blockCount);

/* a cached block */
typedef struct sCBlock {
	struct sCBlock *prev;
	struct sCBlock *next;
	struct sCBlock *hnext;
	size_t blockNo;
	ushort dirty;
	ushort refs;
	/* NULL indicates an unused entry */
	void *buffer;
} sCBlock;

/* all information about a block-cache */
typedef struct {
	void *handle;
	size_t blockCacheSize;
	size_t blockSize;
	fReadBlocks read;
	fWriteBlocks write;
	sCBlock **hashmap;
	sCBlock *oldestBlock;
	sCBlock *newestBlock;
	sCBlock *freeBlocks;
	sCBlock *blockCache;
	void *blockmem;
	ulong blockshm;
	size_t hits;
	size_t misses;
} sBlockCache;

/**
 * Inits the block-cache
 *
 * @param c the block-cache
 * @param fd the file descriptor for the disk device
 */
void bcache_init(sBlockCache *c,int fd);

/**
 * Destroyes the given cache
 *
 * @param c the block-cache
 */
void bcache_destroy(sBlockCache *c);

/**
 * Writes all dirty blocks to disk
 *
 * @param c the block-cache
 */
void bcache_flush(sBlockCache *c);

/**
 * Marks the given block as dirty
 *
 * @param b the block
 */
void bcache_markDirty(sCBlock *b);

/**
 * Creates a new block-cache-entry for given block-number. Does not read the contents from disk!
 * Uses implicitly BMODE_WRITE.
 * Note that you HAVE TO call bcache_release() when you're done!
 *
 * @param c the block-cache
 * @param blockNo the block-number
 * @return the block or NULL
 */
sCBlock *bcache_create(sBlockCache *c,block_t blockNo);

/**
 * Requests the block with given number.
 * Note that you HAVE TO call bcache_release() when you're done!
 *
 * @param c the block-cache
 * @param blockNo the block to fetch from disk or from the cache
 * @param mode the mode (BMODE_*)
 * @return the block or NULL
 */
sCBlock *bcache_request(sBlockCache *c,block_t blockNo,uint mode);

/**
 * Releases the given block
 *
 * @param b the block
 */
void bcache_release(sCBlock *b);

/**
 * Prints statistics about the given blockcache to the given file
 *
 * @param f the file
 * @param c the block-cache
 */
void bcache_printStats(FILE *f,sBlockCache *c);

#if DEBUGGING

/**
 * Prints the used and free blocks
 *
 * @param c the block-cache
 */
void bcache_print(sBlockCache *c);

#endif
