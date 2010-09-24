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
#include <esc/proc.h>
#include <esc/debug.h>
#include <esc/thread.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "ext2.h"
#include "rw.h"
#include "inodecache.h"
#include "../blockcache.h"

#define ALLOC_LOCK	0xF7180001

/**
 * Aquires the lock for given mode and inode. Assumes that ALLOC_LOCK is aquired and releases
 * it at the end.
 */
static void ext2_icache_aquire(sExt2CInode *inode,u8 mode);
/**
 * Releases the given inode
 */
static void ext2_icache_doRelease(sExt2CInode *ino,bool unlockAlloc);
/**
 * Reads the inode from block-cache. Requires inode->inodeNo to be valid!
 */
static void ext2_icache_read(sExt2 *e,sExt2CInode *inode);
/**
 * Writes the inode back to the cached block, which can be written to disk later
 */
static void ext2_icache_write(sExt2 *e,sExt2CInode *inode);

/* for statistics */
static u32 cacheHits = 0;
static u32 cacheMisses = 0;

void ext2_icache_init(sExt2 *e) {
	u32 i;
	sExt2CInode *inode = e->inodeCache;
	for(i = 0; i < EXT2_ICACHE_SIZE; i++) {
		inode->inodeNo = EXT2_BAD_INO;
		inode->refs = 0;
		inode->dirty = false;
		inode++;
	}
}

void ext2_icache_flush(sExt2 *e) {
	sExt2CInode *inode,*end = e->inodeCache + EXT2_ICACHE_SIZE;
	for(inode = e->inodeCache; inode < end; inode++) {
		if(inode->dirty) {
			assert(lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
			ext2_icache_aquire(inode,IMODE_READ);
			ext2_icache_write(e,inode);
			ext2_icache_release(inode);
		}
	}
}

void ext2_icache_markDirty(sExt2CInode *inode) {
	/*vassert(inode->writeRef > 0,"[%d] Inode %d is read-only",gettid(),inode->inodeNo);*/
	inode->dirty = true;
}

sExt2CInode *ext2_icache_request(sExt2 *e,tInodeNo no,u8 mode) {
	/* search for the inode. perhaps it's already in cache */
	sExt2CInode *startNode = e->inodeCache + (no & (EXT2_ICACHE_SIZE - 1));
	sExt2CInode *iend = e->inodeCache + EXT2_ICACHE_SIZE;
	sExt2CInode *inode;
	if(no <= EXT2_BAD_INO)
		return NULL;

	/* lock the request of an inode */
	assert(lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);

	for(inode = startNode; inode < iend; inode++) {
		if(inode->inodeNo == no) {
			ext2_icache_aquire(inode,mode);
			cacheHits++;
			return inode;
		}
	}
	/* look in 0 .. startNode; separate it to make the average-case fast */
	if(inode == iend) {
		for(inode = e->inodeCache; inode < startNode; inode++) {
			if(inode->inodeNo == no) {
				ext2_icache_aquire(inode,mode);
				cacheHits++;
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
		for(inode = e->inodeCache; inode < startNode; inode++) {
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
		ext2_icache_aquire(inode,IMODE_READ);
		ext2_icache_write(e,inode);
		ext2_icache_doRelease(inode,false);
	}

	/* build node */
	inode->inodeNo = no;
	inode->dirty = false;
	/* first for writing because we have to load it */
	ext2_icache_aquire(inode,IMODE_WRITE);

	ext2_icache_read(e,inode);

	/* now use for the requested mode */
	if(mode != IMODE_WRITE) {
		ext2_icache_doRelease(inode,false);
		ext2_icache_aquire(inode,mode);
	}

	cacheMisses++;
	return inode;
}

void ext2_icache_release(const sExt2CInode *inode) {
	ext2_icache_doRelease((sExt2CInode*)inode,true);
	/*debugf("[%d] Released %d for %d\n",gettid(),
			inode->inodeNo,inode->writeRef ? IMODE_WRITE : IMODE_READ);*/
}

static void ext2_icache_aquire(sExt2CInode *inode,u8 mode) {
	inode->refs++;
	assert(unlock(ALLOC_LOCK) == 0);
	assert(lock((u32)inode,(mode & IMODE_WRITE) ? LOCK_EXCLUSIVE : 0) == 0);
	/*debugf("[%d] Aquired %d for %d\n",gettid(),inode->inodeNo,mode);*/
}

static void ext2_icache_doRelease(sExt2CInode *ino,bool unlockAlloc) {
	if(ino == NULL)
		return;

	/* don't write dirty blocks back here, because this would lead to too many writes. */
	/* skipping it until the inode-cache-entry should be reused, is better */
	assert(lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	ino->refs--;
	if(unlockAlloc)
		assert(unlock(ALLOC_LOCK) == 0);
	assert(unlock((u32)ino) == 0);
}

static void ext2_icache_read(sExt2 *e,sExt2CInode *inode) {
	sExt2BlockGrp *group = e->groups + ((inode->inodeNo - 1) / e->superBlock.inodesPerGroup);
	u32 inodesPerBlock = EXT2_BLK_SIZE(e) / sizeof(sExt2Inode);
	u32 noInGroup = (inode->inodeNo - 1) % e->superBlock.inodesPerGroup;
	u32 blockNo = group->inodeTable + noInGroup / inodesPerBlock;
	u32 inodeInBlock = (inode->inodeNo - 1) % inodesPerBlock;
	sCBlock *block = bcache_request(&e->blockCache,blockNo,BMODE_READ);
	vassert(block != NULL,"Fetching block %d failed",blockNo);
	memcpy(&(inode->inode),block->buffer + inodeInBlock * sizeof(sExt2Inode),sizeof(sExt2Inode));
	bcache_release(block);
}

static void ext2_icache_write(sExt2 *e,sExt2CInode *inode) {
	sExt2BlockGrp *group = e->groups + ((inode->inodeNo - 1) / e->superBlock.inodesPerGroup);
	u32 inodesPerBlock = EXT2_BLK_SIZE(e) / sizeof(sExt2Inode);
	u32 noInGroup = (inode->inodeNo - 1) % e->superBlock.inodesPerGroup;
	u32 blockNo = group->inodeTable + noInGroup / inodesPerBlock;
	u32 inodeInBlock = (inode->inodeNo - 1) % inodesPerBlock;
	sCBlock *block = bcache_request(&e->blockCache,blockNo,BMODE_WRITE);
	vassert(block != NULL,"Fetching block %d failed",blockNo);
	memcpy(block->buffer + inodeInBlock * sizeof(sExt2Inode),&(inode->inode),sizeof(sExt2Inode));
	bcache_markDirty(block);
	bcache_release(block);
}

void ext2_icache_printStats(void) {
	printf("[InodeCache] Hits: %d, Misses: %d; %d %%\n",cacheHits,cacheMisses,
			(u32)(100 / ((float)(cacheMisses + cacheHits) / cacheHits)));
}
