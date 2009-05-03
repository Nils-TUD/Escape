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
#include <esc/heap.h>
#include <esc/proc.h>
#include <esc/debug.h>
#include <string.h>

#include "ext2.h"
#include "request.h"
#include "inodecache.h"

/* for statistics */
static u32 cacheHits = 0;
static u32 cacheMisses = 0;

void ext2_icache_init(sExt2 *e) {
	u32 i;
	sCachedInode *inode = e->inodeCache;
	for(i = 0; i < INODE_CACHE_SIZE; i++) {
		inode->refs = 0;
		/* TODO is the -1 really correct? */
		inode->inodeNo = EXT2_BAD_INO - 1;
		inode++;
	}
}

sCachedInode *ext2_icache_request(sExt2 *e,tInodeNo no) {
	tInodeNo noInGroup;
	u32 inodesPerBlock;
	sInode *buffer;
	sBlockGroup *group;

	/* search for the inode. perhaps it's already in cache */
	/* otherwise we try to use not yet used slots at first. If there is no one we use a slot
	 * that is no longer in use. */
	u32 i;
	sCachedInode *ifree = NULL;
	sCachedInode *iusable = NULL;
	sCachedInode *inode = e->inodeCache;
	for(i = 0; i < INODE_CACHE_SIZE; i++) {
		if(inode->inodeNo == no) {
			inode->refs++;
			cacheHits++;
			return inode;
		}
		else if(ifree == NULL && inode->inodeNo == EXT2_BAD_INO)
			ifree = inode;
		else if(iusable == NULL && inode->refs == 0)
			iusable = inode;
		inode++;
	}

	/* determine slot to use */
	if(ifree != NULL)
		inode = ifree;
	else if(iusable != NULL)
		inode = iusable;
	else {
		debugf("NO FREE INODE-CACHE-SLOT! What to to??");
		return NULL;
	}

	/* read block with that inode from disk */
	inodesPerBlock = BLOCK_SIZE(e) / sizeof(sInode);
	buffer = (sInode*)malloc(BLOCK_SIZE(e));
	group = e->groups + ((no - 1) / e->superBlock.inodesPerGroup);
	noInGroup = (no - 1) % e->superBlock.inodesPerGroup;
	if(!ext2_readBlocks(e,(u8*)buffer,group->inodeTable + noInGroup / inodesPerBlock,1))
		return NULL;

	/* build node */
	inode->inodeNo = no;
	inode->refs = 1;
	memcpy(&(inode->inode),buffer + (noInGroup % inodesPerBlock),sizeof(sInode));
	free(buffer);

	cacheMisses++;
	return inode;
}

void ext2_icache_release(sExt2 *e,sCachedInode *inode) {
	UNUSED(e);
	if(inode->refs > 0)
		inode->refs--;
}

void ext2_icache_printStats(void) {
	debugf("[InodeCache] Hits: %d, Misses: %d; %d %%\n",cacheHits,cacheMisses,
			(u32)(100 / ((float)(cacheMisses + cacheHits) / cacheHits)));
}
