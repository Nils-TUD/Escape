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
#include <esc/proc.h>
#include <esc/debug.h>
#include <esc/thread.h>
#include <esc/endian.h>
#include <fs/blockcache.h>
#include <fs/fsdev.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "ext2.h"
#include "file.h"
#include "rw.h"
#include "inodecache.h"

Ext2INodeCache::Ext2INodeCache(Ext2FileSystem *fs)
		: _hits(), _misses(), _cache(new Ext2CInode[EXT2_ICACHE_SIZE]), _fs(fs) {
	size_t i;
	Ext2CInode *inode = _cache;
	for(i = 0; i < EXT2_ICACHE_SIZE; i++) {
		inode->inodeNo = EXT2_BAD_INO;
		inode->refs = 0;
		inode->dirty = false;
		inode++;
	}
}

void Ext2INodeCache::flush() {
	Ext2CInode *inode,*end = _cache + EXT2_ICACHE_SIZE;
	for(inode = _cache; inode < end; inode++) {
		if(inode->dirty) {
			assert(tpool_lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
			acquire(inode,IMODE_READ);
			write(inode);
			release(inode);
		}
	}
}

Ext2CInode *Ext2INodeCache::request(inode_t no,uint mode) {
	/* search for the inode. perhaps it's already in cache */
	Ext2CInode *startNode = _cache + (no & (EXT2_ICACHE_SIZE - 1));
	Ext2CInode *iend = _cache + EXT2_ICACHE_SIZE;
	Ext2CInode *inode;
	if(no <= EXT2_BAD_INO)
		return NULL;

	/* tpool_lock the request of an inode */
	assert(tpool_lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);

	for(inode = startNode; inode < iend; inode++) {
		if(inode->inodeNo == no) {
			acquire(inode,mode);
			_hits++;
			return inode;
		}
	}
	/* look in 0 .. startNode; separate it to make the average-case fast */
	if(inode == iend) {
		for(inode = _cache; inode < startNode; inode++) {
			if(inode->inodeNo == no) {
				acquire(inode,mode);
				_hits++;
				return inode;
			}
		}
	}

	/* ok, not in cache. so we start again at the position to find a usable node */
	/* if we have to load it from disk anyway I think we can waste a few cycles more
	 * to reduce the number of cycles in the cache-lookup. therefore
	 * we don't collect the information in the loops above. */
	for(inode = startNode; inode < iend; inode++) {
		if(inode->inodeNo == EXT2_BAD_INO || inode->refs == 0)
			break;
	}
	if(inode == iend) {
		for(inode = _cache; inode < startNode; inode++) {
			if(inode->inodeNo == EXT2_BAD_INO || inode->refs == 0)
				break;
		}

		if(inode == startNode) {
			printf("NO FREE INODE-CACHE-SLOT! What to to??");
			return NULL;
		}
	}

	/* write the old inode back, if necessary */
	if(inode->dirty && inode->inodeNo != EXT2_BAD_INO) {
		acquire(inode,IMODE_READ);
		write(inode);
		doRelease(inode,false);
	}

	/* build node */
	inode->inodeNo = no;
	inode->dirty = false;
	/* first for writing because we have to load it */
	acquire(inode,IMODE_WRITE);

	read(inode);

	/* now use for the requested mode */
	if(mode != IMODE_WRITE) {
		doRelease(inode,false);
		acquire(inode,mode);
	}

	_misses++;
	return inode;
}

void Ext2INodeCache::print(FILE *f) {
	float hitrate;
	size_t used = 0,dirty = 0;
	Ext2CInode *inode,*end = _cache + EXT2_ICACHE_SIZE;
	for(inode = _cache; inode < end; inode++) {
		if(inode->inodeNo != EXT2_BAD_INO)
			used++;
		if(inode->dirty)
			dirty++;
	}
	fprintf(f,"\t\tTotal entries: %u\n",EXT2_ICACHE_SIZE);
	fprintf(f,"\t\tUsed entries: %zu\n",used);
	fprintf(f,"\t\tDirty entries: %zu\n",dirty);
	fprintf(f,"\t\tHits: %zu\n",_hits);
	fprintf(f,"\t\tMisses: %zu\n",_misses);
	if(_hits == 0)
		hitrate = 0;
	else
		hitrate = 100.0f / ((float)(_misses + _hits) / _hits);
	fprintf(f,"\t\tHitrate: %.3f%%\n",hitrate);
}

void Ext2INodeCache::acquire(Ext2CInode *inode,A_UNUSED uint mode) {
	inode->refs++;
	assert(tpool_unlock(ALLOC_LOCK) == 0);
	assert(tpool_lock((uint)inode,(mode & IMODE_WRITE) ? LOCK_EXCLUSIVE : 0) == 0);
}

void Ext2INodeCache::doRelease(Ext2CInode *ino,bool unlockAlloc) {
	if(ino == NULL)
		return;

	/* don't write dirty blocks back here, because this would lead to too many writes. */
	/* skipping it until the inode-cache-entry should be reused, is better */
	assert(tpool_lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	/* if there are no references and no links anymore, we have to delete the file */
	if(--ino->refs == 0) {
		if(ino->inode.linkCount == 0) {
			Ext2File::remove(_fs,ino);
			/* ensure that we don't use the cached inode again */
			ino->inodeNo = EXT2_BAD_INO;
			ino->dirty = false;
		}
	}
	if(unlockAlloc)
		assert(tpool_unlock(ALLOC_LOCK) == 0);
	assert(tpool_unlock((uint)ino) == 0);
}

void Ext2INodeCache::read(Ext2CInode *inode) {
	uint32_t inodesPerGroup = le32tocpu(_fs->sb.get()->inodesPerGroup);
	Ext2BlockGrp *group = _fs->bgs.get((inode->inodeNo - 1) / inodesPerGroup);
	size_t inodesPerBlock = _fs->blockSize() / sizeof(Ext2Inode);
	size_t noInGroup = (inode->inodeNo - 1) % inodesPerGroup;
	block_t blockNo = le32tocpu(group->inodeTable) + noInGroup / inodesPerBlock;
	size_t inodeInBlock = (inode->inodeNo - 1) % inodesPerBlock;
	CBlock *block = _fs->blockCache.request(blockNo,BlockCache::READ);
	vassert(block != NULL,"Fetching block %d failed",blockNo);
	memcpy(&(inode->inode),(uint8_t*)block->buffer + inodeInBlock * sizeof(Ext2Inode),
			sizeof(Ext2Inode));
	_fs->blockCache.release(block);
}

void Ext2INodeCache::write(Ext2CInode *inode) {
	uint32_t inodesPerGroup = le32tocpu(_fs->sb.get()->inodesPerGroup);
	Ext2BlockGrp *group = _fs->bgs.get((inode->inodeNo - 1) / inodesPerGroup);
	size_t inodesPerBlock = _fs->blockSize() / sizeof(Ext2Inode);
	size_t noInGroup = (inode->inodeNo - 1) % inodesPerGroup;
	block_t blockNo = le32tocpu(group->inodeTable) + noInGroup / inodesPerBlock;
	size_t inodeInBlock = (inode->inodeNo - 1) % inodesPerBlock;
	CBlock *block = _fs->blockCache.request(blockNo,BlockCache::WRITE);
	vassert(block != NULL,"Fetching block %d failed",blockNo);
	memcpy((uint8_t*)block->buffer + inodeInBlock * sizeof(Ext2Inode),&(inode->inode),
			sizeof(Ext2Inode));
	_fs->blockCache.markDirty(block);
	_fs->blockCache.release(block);
}
