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

#pragma once

#include <esc/common.h>
#include <stdio.h>

/* a cached block */
struct CBlock {
	CBlock *prev;
	CBlock *next;
	CBlock *hnext;
	size_t blockNo;
	ushort dirty;
	ushort refs;
	/* NULL indicates an unused entry */
	void *buffer;
};

class BlockCache {
	static const size_t HASH_SIZE	= 256;

public:
	enum {
		READ	= 0x1,
		WRITE	= 0x2,
	};

	/**
	 * Inits the block-cache
	 *
	 * @param fd the file descriptor for the disk device
	 * @param blocks the number of blocks in the cache
	 * @param bsize the block size
	 */
	explicit BlockCache(int fd,size_t blocks,size_t bsize);

	/**
	 * Destroyes the given cache
	 */
	~BlockCache();

	/**
	 * Reads <blockCount> blocks beginning with <start> into <buffer>.
	 *
	 * @param buffer the buffer to write to
	 * @param start he start block number
	 * @param blockCount the number of blocks
	 * @return if successfull
	 */
	virtual bool readBlocks(void *buffer,block_t start,size_t blockCount) = 0;

	/**
	 * Writes <blockCount> blocks beginning with <start> from <buffer>.
	 *
	 * @param buffer the buffer to write
	 * @param start he start block number
	 * @param blockCount the number of blocks
	 * @return if successfull
	 */
	virtual bool writeBlocks(const void *buffer,size_t start,size_t blockCount) = 0;

	/**
	 * Writes all dirty blocks to disk
	 */
	void flush();

	/**
	 * Marks the given block as dirty
	 *
	 * @param b the block
	 */
	void markDirty(CBlock *b) {
		b->dirty = true;
	}

	/**
	 * Creates a new block-cache-entry for given block-number. Does not read the contents from disk!
	 * Uses implicitly BMODE_WRITE.
	 * Note that you HAVE TO call release() when you're done!
	 *
	 * @param blockNo the block-number
	 * @return the block or NULL
	 */
	CBlock *create(block_t blockNo) {
		return doRequest(blockNo,false,WRITE);
	}

	/**
	 * Requests the block with given number.
	 * Note that you HAVE TO call release() when you're done!
	 *
	 * @param blockNo the block to fetch from disk or from the cache
	 * @param mode the mode (BMODE_*)
	 * @return the block or NULL
	 */
	CBlock *request(block_t blockNo,uint mode) {
		return doRequest(blockNo,true,mode);
	}

	/**
	 * Releases the given block
	 *
	 * @param b the block
	 */
	void release(CBlock *b) {
		doRelease(b,true);
	}

	/**
	 * Prints statistics about the given blockcache to the given file
	 *
	 * @param f the file
	 */
	void printStats(FILE *f);

#if DEBUGGING

	/**
	 * Prints the used and free blocks
	 */
	void print();

#endif

private:
	/**
	 * Aquires the tpool_lock, depending on <mode>, for the given block
	 */
	void acquire(CBlock *b,uint mode);
	/**
	 * Releases the tpool_lock for given block
	 */
	void doRelease(CBlock *b,bool unlockAlloc);
	/**
	 * Requests the given block and reads it from disk if desired
	 */
	CBlock *doRequest(block_t blockNo,bool doRead,uint mode);
	/**
	 * Fetches a block-cache-entry
	 */
	CBlock *getBlock(block_t blockNo);

	size_t _blockCacheSize;
	size_t _blockSize;
	CBlock **_hashmap;
	CBlock *_oldestBlock;
	CBlock *_newestBlock;
	CBlock *_freeBlocks;
	CBlock *_blockCache;
	void *_blockmem;
	ulong _blockshm;
	ulong _hits;
	ulong _misses;
};
