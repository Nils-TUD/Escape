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

#include <sys/common.h>
#include <sys/mem/pagedir.h>
#include <sys/spinlock.h>
#include <assert.h>

#define INVALID_BLOCK		0xFFFFFFFF

class SwapMap {
	SwapMap() = delete;

	struct Block {
		uint refCount;
		Block *next;
	};

public:
	/**
	 * Inits the swap-map
	 *
	 * @param swapSize the size of the swap-device in bytes
	 * @return true on success
	 */
	static bool init(size_t swapSize);

	/**
	 * Allocates 1 block on the swap-device
	 *
	 * @return the starting block on the swap-device or INVALID_BLOCK if no free space is left
	 */
	static ulong alloc();

	/**
	 * Increases the references of the given block
	 *
	 * @param the block-number
	 */
	static void incRefs(ulong block);

	/**
	 * @param block the block-number
	 * @return true if the given block is used
	 */
	static bool isUsed(ulong block);

	/**
	 * @return the total space on the swap device in bytes
	 */
	static size_t totalSpace() {
		return totalBlocks * PAGE_SIZE;
	}

	/**
	 * @return the free space in bytes
	 */
	static size_t freeSpace() {
		return freeBlocks * PAGE_SIZE;
	}

	/**
	 * Free's the given block
	 *
	 * @param block the block to free
	 */
	static void free(ulong block);

	/**
	 * Prints the swap-map
	 *
	 * @param os the output-stream
	 */
	static void print(OStream &os);

private:
	static size_t totalBlocks;
	static size_t freeBlocks;
	static Block *swapBlocks;
	static Block *freeList;
	static klock_t lock;
};

inline void SwapMap::incRefs(ulong block) {
	SpinLock::acquire(&lock);
	assert(block < totalBlocks && swapBlocks[block].refCount > 0);
	swapBlocks[block].refCount++;
	SpinLock::release(&lock);
}

inline bool SwapMap::isUsed(ulong block) {
	bool res;
	SpinLock::acquire(&lock);
	assert(block < totalBlocks);
	res = swapBlocks[block].refCount > 0;
	SpinLock::release(&lock);
	return res;
}
