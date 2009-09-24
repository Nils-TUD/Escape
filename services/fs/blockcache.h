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

#ifndef BLOCKCACHE_H_
#define BLOCKCACHE_H_

#include <esc/common.h>

/* reading/writing of blocks */
typedef bool (*fReadBlocks)(void *h,void *buffer,u32 start,u16 blockCount);
typedef bool (*fWriteBlocks)(void *h,const void *buffer,u32 start,u16 blockCount);

/* a cached block */
typedef struct sCBlock {
	struct sCBlock *prev;
	struct sCBlock *next;
	u32 blockNo;
	u8 dirty;
	/* NULL indicates an unused entry */
	u8 *buffer;
} sCBlock;

/* all information about a block-cache */
typedef struct {
	void *handle;
	u32 blockCacheSize;
	u32 blockSize;
	fReadBlocks read;
	fWriteBlocks write;
	sCBlock *usedBlocks;
	sCBlock *oldestBlock;
	sCBlock *freeBlocks;
	sCBlock *blockCache;
} sBlockCache;

/**
 * Inits the block-cache
 *
 * @param c the block-cache
 */
void bcache_init(sBlockCache *c);

/**
 * Writes all dirty blocks to disk
 *
 * @param c the block-cache
 */
void bcache_flush(sBlockCache *c);

/**
 * Creates a new block-cache-entry for given block-number. Does not read the contents from disk!
 *
 * @param c the block-cache
 * @param blockNo the block-number
 * @return the block or NULL
 */
sCBlock *bcache_create(sBlockCache *c,u32 blockNo);

/**
 * Requests the block with given number
 *
 * @param c the block-cache
 * @param blockNo the block to fetch from disk or from the cache
 * @return the block or NULL
 */
sCBlock *bcache_request(sBlockCache *c,u32 blockNo);

#if DEBUGGING

/**
 * Prints the used and free blocks
 *
 * @param c the block-cache
 */
void bcache_print(sBlockCache *c);

/**
 * Prints block-cache statistics
 */
void bcache_printStats(void);

#endif

#endif /* BLOCKCACHE_H_ */
