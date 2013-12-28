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

#include <esc/common.h>
#include <esc/proc.h>
#include <esc/debug.h>
#include <esc/thread.h>
#include <esc/endian.h>
#include <fs/blockcache.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "ext2.h"
#include "file.h"
#include "rw.h"
#include "inodecache.h"

#define ALLOC_LOCK	0xF7180001

/**
 * Aquires the tpool_lock for given mode and inode. Assumes that ALLOC_LOCK is acquired and releases
 * it at the end.
 */
static void ext2_icache_acquire(sExt2CInode *inode,uint mode);
/**
 * Releases the given inode
 */
static void ext2_icache_doRelease(sExt2 *e,sExt2CInode *ino,bool unlockAlloc);
/**
 * Reads the inode from block-cache. Requires inode->inodeNo to be valid!
 */
static void ext2_icache_read(sExt2 *e,sExt2CInode *inode);
/**
 * Writes the inode back to the cached block, which can be written to disk later
 */
static void ext2_icache_write(sExt2 *e,sExt2CInode *inode);

void ext2_icache_init(sExt2 *e) {
	size_t i;
	sExt2CInode *inode = e->inodeCache;
	for(i = 0; i < EXT2_ICACHE_SIZE; i++) {
		inode->inodeNo = EXT2_BAD_INO;
		inode->refs = 0;
		inode->dirty = false;
		inode++;
	}
	e->icacheHits = 0;
	e->icacheMisses = 0;
}

void ext2_icache_flush(sExt2 *e) {
	sExt2CInode *inode,*end = e->inodeCache + EXT2_ICACHE_SIZE;
	for(inode = e->inodeCache; inode < end; inode++) {
		if(inode->dirty) {
			assert(tpool_lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
			ext2_icache_acquire(inode,IMODE_READ);
			ext2_icache_write(e,inode);
			ext2_icache_release(e,inode);
		}
	}
}

void ext2_icache_markDirty(sExt2CInode *inode) {
	inode->dirty = true;
}

sExt2CInode *ext2_icache_request(sExt2 *e,inode_t no,uint mode) {
	/* search for the inode. perhaps it's already in cache */
	sExt2CInode *startNode = e->inodeCache + (no & (EXT2_ICACHE_SIZE - 1));
	sExt2CInode *iend = e->inodeCache + EXT2_ICACHE_SIZE;
	sExt2CInode *inode;
	if(no <= EXT2_BAD_INO)
		return NULL;

	/* tpool_lock the request of an inode */
	assert(tpool_lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);

	for(inode = startNode; inode < iend; inode++) {
		if(inode->inodeNo == no) {
			ext2_icache_acquire(inode,mode);
			e->icacheHits++;
			return inode;
		}
	}
	/* look in 0 .. startNode; separate it to make the average-case fast */
	if(inode == iend) {
		for(inode = e->inodeCache; inode < startNode; inode++) {
			if(inode->inodeNo == no) {
				ext2_icache_acquire(inode,mode);
				e->icacheHits++;
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
		ext2_icache_acquire(inode,IMODE_READ);
		ext2_icache_write(e,inode);
		ext2_icache_doRelease(e,inode,false);
	}

	/* build node */
	inode->inodeNo = no;
	inode->dirty = false;
	/* first for writing because we have to load it */
	ext2_icache_acquire(inode,IMODE_WRITE);

	ext2_icache_read(e,inode);

	/* now use for the requested mode */
	if(mode != IMODE_WRITE) {
		ext2_icache_doRelease(e,inode,false);
		ext2_icache_acquire(inode,mode);
	}

	e->icacheMisses++;
	return inode;
}

void ext2_icache_release(sExt2 *e,const sExt2CInode *inode) {
	ext2_icache_doRelease(e,(sExt2CInode*)inode,true);
}

void ext2_icache_print(FILE *f,sExt2 *e) {
	float hitrate;
	size_t used = 0,dirty = 0;
	sExt2CInode *inode,*end = e->inodeCache + EXT2_ICACHE_SIZE;
	for(inode = e->inodeCache; inode < end; inode++) {
		if(inode->inodeNo != EXT2_BAD_INO)
			used++;
		if(inode->dirty)
			dirty++;
	}
	fprintf(f,"\t\tTotal entries: %u\n",EXT2_ICACHE_SIZE);
	fprintf(f,"\t\tUsed entries: %u\n",used);
	fprintf(f,"\t\tDirty entries: %u\n",dirty);
	fprintf(f,"\t\tHits: %u\n",e->icacheHits);
	fprintf(f,"\t\tMisses: %u\n",e->icacheMisses);
	if(e->icacheHits == 0)
		hitrate = 0;
	else
		hitrate = 100.0f / ((float)(e->icacheMisses + e->icacheHits) / e->icacheHits);
	fprintf(f,"\t\tHitrate: %.3f%%\n",hitrate);
}

static void ext2_icache_acquire(sExt2CInode *inode,A_UNUSED uint mode) {
	inode->refs++;
	assert(tpool_unlock(ALLOC_LOCK) == 0);
	assert(tpool_lock((uint)inode,(mode & IMODE_WRITE) ? LOCK_EXCLUSIVE : 0) == 0);
}

static void ext2_icache_doRelease(sExt2 *e,sExt2CInode *ino,bool unlockAlloc) {
	if(ino == NULL)
		return;

	/* don't write dirty blocks back here, because this would lead to too many writes. */
	/* skipping it until the inode-cache-entry should be reused, is better */
	assert(tpool_lock(ALLOC_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	/* if there are no references and no links anymore, we have to delete the file */
	if(--ino->refs == 0) {
		if(ino->inode.linkCount == 0) {
			ext2_file_delete(e,ino);
			/* ensure that we don't use the cached inode again */
			ino->inodeNo = EXT2_BAD_INO;
			ino->dirty = false;
		}
	}
	if(unlockAlloc)
		assert(tpool_unlock(ALLOC_LOCK) == 0);
	assert(tpool_unlock((uint)ino) == 0);
}

static void ext2_icache_read(sExt2 *e,sExt2CInode *inode) {
	uint32_t inodesPerGroup = le32tocpu(e->superBlock.inodesPerGroup);
	sExt2BlockGrp *group = e->groups + ((inode->inodeNo - 1) / inodesPerGroup);
	size_t inodesPerBlock = EXT2_BLK_SIZE(e) / sizeof(sExt2Inode);
	size_t noInGroup = (inode->inodeNo - 1) % inodesPerGroup;
	block_t blockNo = le32tocpu(group->inodeTable) + noInGroup / inodesPerBlock;
	size_t inodeInBlock = (inode->inodeNo - 1) % inodesPerBlock;
	sCBlock *block = bcache_request(&e->blockCache,blockNo,BMODE_READ);
	vassert(block != NULL,"Fetching block %d failed",blockNo);
	memcpy(&(inode->inode),(uint8_t*)block->buffer + inodeInBlock * sizeof(sExt2Inode),
			sizeof(sExt2Inode));
	bcache_release(block);
}

static void ext2_icache_write(sExt2 *e,sExt2CInode *inode) {
	uint32_t inodesPerGroup = le32tocpu(e->superBlock.inodesPerGroup);
	sExt2BlockGrp *group = e->groups + ((inode->inodeNo - 1) / inodesPerGroup);
	size_t inodesPerBlock = EXT2_BLK_SIZE(e) / sizeof(sExt2Inode);
	size_t noInGroup = (inode->inodeNo - 1) % inodesPerGroup;
	block_t blockNo = le32tocpu(group->inodeTable) + noInGroup / inodesPerBlock;
	size_t inodeInBlock = (inode->inodeNo - 1) % inodesPerBlock;
	sCBlock *block = bcache_request(&e->blockCache,blockNo,BMODE_WRITE);
	vassert(block != NULL,"Fetching block %d failed",blockNo);
	memcpy((uint8_t*)block->buffer + inodeInBlock * sizeof(sExt2Inode),&(inode->inode),
			sizeof(sExt2Inode));
	bcache_markDirty(block);
	bcache_release(block);
}
