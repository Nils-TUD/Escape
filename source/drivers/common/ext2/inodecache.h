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

#include "inode.h"

class Ext2FileSystem;

struct Ext2CInode {
	inode_t inodeNo;
	ushort dirty;
	ushort refs;
	Ext2Inode inode;
};

enum {
	IMODE_READ	= 0x1,
	IMODE_WRITE	= 0x2,
};

class Ext2INodeCache {
public:
	/**
	 * Inits the inode-cache
	 */
	explicit Ext2INodeCache(Ext2FileSystem *fs);
	~Ext2INodeCache() {
		delete[] _cache;
	}

	/**
	 * Writes all dirty inodes to disk
	 */
	void flush();

	/**
	 * Marks the given inode dirty
	 *
	 * @param inode the inode
	 */
	void markDirty(Ext2CInode *inode) {
		inode->dirty = true;
	}

	/**
	 * Requests the inode with given number. That means if it is in the cache you'll simply get it.
	 * Otherwise it is fetched from disk and put into the cache. The references of the cache-inode
	 * will be increased.
	 *
	 * @param no the inode-number
	 * @param mode the mode: IMODE_*
	 * @return the cached node or NULL
	 */
	Ext2CInode *request(inode_t no,uint mode);

	/**
	 * Releases the given inode. That means the references will be decreased and the inode will be
	 * removed from cache if there are no more references.
	 *
	 * @param inode the inode
	 */
	void release(const Ext2CInode *inode) {
		doRelease((Ext2CInode*)inode,true);
	}

	/**
	 * Prints statistics and information about the inode-cache into the givne file
	 *
	 * @param f the file
	 */
	void print(FILE *f);

private:
	/**
	 * Aquires the tpool_lock for given mode and inode. Assumes that ALLOC_LOCK is acquired and releases
	 * it at the end.
	 */
	void acquire(Ext2CInode *inode,uint mode);
	/**
	 * Releases the given inode
	 */
	void doRelease(Ext2CInode *ino,bool unlockAlloc);
	/**
	 * Reads the inode from block-cache. Requires inode->inodeNo to be valid!
	 */
	void read(Ext2CInode *inode);
	/**
	 * Writes the inode back to the cached block, which can be written to disk later
	 */
	void write(Ext2CInode *inode);

	size_t _hits;
	size_t _misses;
	Ext2CInode *_cache;
	Ext2FileSystem *_fs;
};
