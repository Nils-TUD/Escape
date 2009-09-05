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
#include "rw.h"
#include "inodecache.h"
#include "blockcache.h"

/**
 * Reads the inode from block-cache. Requires inode->inodeNo to be valid!
 */
static void ext2_icache_read(sExt2 *e,sCachedInode *inode);
/**
 * Writes the inode back to the cached block, which can be written to disk later
 */
static void ext2_icache_write(sExt2 *e,sCachedInode *inode);

/* for statistics */
static u32 cacheHits = 0;
static u32 cacheMisses = 0;

void ext2_icache_init(sExt2 *e) {
	u32 i;
	sCachedInode *inode = e->inodeCache;
	for(i = 0; i < INODE_CACHE_SIZE; i++) {
		inode->refs = 0;
		inode->inodeNo = EXT2_BAD_INO;
		inode->dirty = false;
		inode++;
	}
}

void ext2_icache_flush(sExt2 *e) {
	sCachedInode *inode,*end = e->inodeCache + INODE_CACHE_SIZE;
	for(inode = e->inodeCache; inode < end; inode++) {
		if(inode->dirty)
			ext2_icache_write(e,inode);
	}
}

sCachedInode *ext2_icache_request(sExt2 *e,tInodeNo no) {
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

	/* build node */
	inode->inodeNo = no;
	inode->refs = 1;
	inode->dirty = false;
	ext2_icache_read(e,inode);

	cacheMisses++;
	return inode;
}

void ext2_icache_release(sExt2 *e,sCachedInode *inode) {
	UNUSED(e);
	if(inode == NULL)
		return;
	if(inode->refs > 0) {
		/* write it back, if we free it */
		if(--inode->refs == 0)
			ext2_icache_write(e,inode);
	}
}

static void ext2_icache_read(sExt2 *e,sCachedInode *inode) {
	sBlockGroup *group = e->groups + ((inode->inodeNo - 1) / e->superBlock.inodesPerGroup);
	u32 inodesPerBlock = BLOCK_SIZE(e) / sizeof(sInode);
	u32 noInGroup = (inode->inodeNo - 1) % e->superBlock.inodesPerGroup;
	sBCacheEntry *block = ext2_bcache_request(e,group->inodeTable + noInGroup / inodesPerBlock);
	memcpy(&(inode->inode),block->buffer + ((inode->inodeNo - 1) % inodesPerBlock) * sizeof(sInode),
			sizeof(sInode));
}

static void ext2_icache_write(sExt2 *e,sCachedInode *inode) {
	sBlockGroup *group = e->groups + ((inode->inodeNo - 1) / e->superBlock.inodesPerGroup);
	u32 inodesPerBlock = BLOCK_SIZE(e) / sizeof(sInode);
	u32 noInGroup = (inode->inodeNo - 1) % e->superBlock.inodesPerGroup;
	sBCacheEntry *block = ext2_bcache_request(e,group->inodeTable + noInGroup / inodesPerBlock);
	memcpy(block->buffer + ((inode->inodeNo - 1) % inodesPerBlock) * sizeof(sInode),
			&(inode->inode),sizeof(sInode));
	block->dirty = true;
	inode->dirty = false;
}

void ext2_icache_printStats(void) {
	debugf("[InodeCache] Hits: %d, Misses: %d; %d %%\n",cacheHits,cacheMisses,
			(u32)(100 / ((float)(cacheMisses + cacheHits) / cacheHits)));
}
