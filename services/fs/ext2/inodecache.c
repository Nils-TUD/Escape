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
		inode->inodeNo = EXT2_BAD_INO;
		inode++;
	}
}

sCachedInode *ext2_icache_request(sExt2 *e,tInodeNo no) {
	tInodeNo noInGroup;
	u32 inodesPerBlock;
	sInode *buffer;
	sBlockGroup *group;

	/* search for the inode. perhaps it's already in cache */
	sCachedInode *startNode = e->inodeCache + (no & (INODE_CACHE_SIZE - 1));
	sCachedInode *iend = e->inodeCache + INODE_CACHE_SIZE;
	sCachedInode *inode;
	for(inode = startNode; inode < iend; inode++) {
		if(inode->inodeNo == no) {
			inode->refs++;
			cacheHits++;
			return inode;
		}
	}
	/* look in 0 .. startNode; separate it to make the average-case fast */
	if(inode == iend) {
		for(inode = e->inodeCache; inode < startNode; inode++) {
			if(inode->inodeNo == no) {
				inode->refs++;
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
			debugf("NO FREE INODE-CACHE-SLOT! What to to??");
			return NULL;
		}
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
