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
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/heap.h>
#include "ext2.h"
#include "request.h"
#include "blockcache.h"
#include "inodecache.h"
#include "superblock.h"

/**
 * Allocates a block in the given block-group
 */
static u32 ext2_allocBlockIn(sExt2 *e,u32 groupStart,sBlockGroup *group);
/**
 * Determines if the given block-group should contain a backup of the super-block
 * and block-group-descriptor-table
 */
static bool ext2_bgContainsBackups(sExt2 *e,u32 i);
/**
 * Checks wether x is a power of y
 */
static bool ext2_isPowerOf(u32 x,u32 y);

bool ext2_initSuperBlock(sExt2 *e) {
	u32 bcount;
	e->sbDirty = false;
	/* read sector-based because we need the super-block to be able to read block-based */
	if(!ext2_readSectors(e,&(e->superBlock),EXT2_SUPERBLOCK_SECNO,2)) {
		printe("Unable to read super-block");
		return false;
	}

	/* check features */
	/* TODO mount readonly if an unsupported feature is present in featureRoCompat */
	if(e->superBlock.featureInCompat || e->superBlock.featureRoCompat) {
		printe("Unable to use filesystem: Incompatible features");
		return false;
	}

	/* read block-group-descriptors */
	bcount = BYTES_TO_BLOCKS(e,ext2_getBlockGroupCount(e));
	e->groupsDirty = false;
	e->groups = (sBlockGroup*)malloc(bcount * BLOCK_SIZE(e));
	if(e->groups == NULL) {
		printe("Unable to allocate memory for blockgroups");
		return false;
	}
	if(!ext2_readBlocks(e,e->groups,e->superBlock.firstDataBlock + 1,bcount)) {
		free(e->groups);
		printe("Unable to read group-table");
		return false;
	}
	return true;
}

void ext2_sync(sExt2 *e) {
	ext2_updateSuperBlock(e);
	ext2_updateBlockGroups(e);
	/* flush inodes first, because they may create dirty blocks */
	ext2_icache_flush(e);
	ext2_bcache_flush(e);
}

u32 ext2_getBlockOfInode(sExt2 *e,tInodeNo inodeNo) {
	return (inodeNo - 1) / e->superBlock.inodesPerGroup;
}

u32 ext2_getGroupOfBlock(sExt2 *e,u32 block) {
	return block / e->superBlock.blocksPerGroup;
}

u32 ext2_allocBlock(sExt2 *e,sCachedInode *inode) {
	u32 gcount = ext2_getBlockGroupCount(e);
	u32 block = ext2_getBlockOfInode(e,inode->inodeNo);
	u32 group = ext2_getGroupOfBlock(e,block);
	u32 i,bno;

	if(e->superBlock.freeBlockCount == 0)
		return 0;

	/* first try to find a block in the block-group of the inode */
	bno = ext2_allocBlockIn(e,group * e->superBlock.blocksPerGroup,e->groups + group);
	if(bno != 0)
		return bno;

	/* now try the other block-groups */
	for(i = bno + 1; i != group; i = (i + 1) % gcount) {
		bno = ext2_allocBlockIn(e,i * e->superBlock.blocksPerGroup,e->groups + i);
		if(bno != 0)
			return bno;
	}
	return 0;
}

s32 ext2_freeBlock(sExt2 *e,u32 blockNo) {
	u32 group = ext2_getGroupOfBlock(e,blockNo);
	sBCacheEntry *bitmap = ext2_bcache_request(e,e->groups[group].blockBitmap);
	if(bitmap == NULL)
		return -1;

	/* mark free in bitmap */
	blockNo--;
	blockNo %= e->superBlock.blocksPerGroup;
	bitmap->buffer[blockNo / 8] &= ~(1 << (blockNo % 8));
	bitmap->dirty = true;
	e->groups[group].freeBlockCount++;
	e->groupsDirty = true;
	e->superBlock.freeBlockCount++;
	e->sbDirty = true;
	return 0;
}

static u32 ext2_allocBlockIn(sExt2 *e,u32 groupStart,sBlockGroup *group) {
	u32 i,j,bno;
	if(group->freeBlockCount == 0)
		return 0;

	/* load bitmap */
	sBCacheEntry *bitmap = ext2_bcache_request(e,group->blockBitmap);
	if(bitmap == NULL)
		return 0;

	bno = 0;
	for(i = 0; i < (u32)BLOCK_SIZE(e); i++) {
		for(j = 1; j < 256; bno++, j <<= 1) {
			if(bno >= e->superBlock.blockCount)
				return 0;
			if(!(bitmap->buffer[i] & j)) {
				group->freeBlockCount--;
				e->groupsDirty = true;
				bitmap->buffer[i] |= j;
				bitmap->dirty = true;
				e->superBlock.freeBlockCount--;
				e->sbDirty = true;
				return bno + groupStart + 1;
			}
		}
	}
	return 0;
}

void ext2_updateSuperBlock(sExt2 *e) {
	u32 i,count,bno;
	if(!e->sbDirty)
		return;

	/* update main superblock; sector-based because we want to update the super-block without
	 * anything else (this would happen if the superblock is in the block 0 together with
	 * the bootrecord) */
	if(!ext2_writeSectors(e,&(e->superBlock),EXT2_SUPERBLOCK_SECNO,2)) {
		printe("Unable to update superblock in blockgroup 0");
		return;
	}

	/* update superblock backups */
	bno = e->superBlock.blocksPerGroup;
	count = ext2_getBlockGroupCount(e);
	for(i = 1; i < count; i++) {
		if(ext2_bgContainsBackups(e,i)) {
			if(!ext2_writeBlocks(e,&(e->superBlock),bno,1)) {
				printe("Unable to update superblock in blockgroup %d",i);
				return;
			}
		}
		bno += e->superBlock.blocksPerGroup;
	}

	/* now we're in sync */
	e->sbDirty = false;
}

void ext2_updateBlockGroups(sExt2 *e) {
	u32 i,bno,count,bcount;
	if(!e->groupsDirty)
		return;

	bcount = BYTES_TO_BLOCKS(e,ext2_getBlockGroupCount(e));

	/* update main block-group-descriptor-table */
	if(!ext2_writeBlocks(e,e->groups,e->superBlock.firstDataBlock + 1,bcount)) {
		printe("Unable to update block-group-descriptor-table in blockgroup 0");
		return;
	}

	/* update block-group-descriptor backups */
	bno = e->superBlock.blocksPerGroup + EXT2_BLOGRPTBL_BNO;
	count = ext2_getBlockGroupCount(e);
	for(i = 1; i < count; i++) {
		if(ext2_bgContainsBackups(e,i)) {
			if(!ext2_writeBlocks(e,e->groups,bno,bcount)) {
				printe("Unable to update block-group-descriptor-table in blockgroup %d",i);
				return;
			}
		}
		bno += e->superBlock.blocksPerGroup;
	}

	/* now we're in sync */
	e->groupsDirty = false;
}

u32 ext2_getBlockGroupCount(sExt2 *e) {
	u32 bpg = e->superBlock.blocksPerGroup;
	return (e->superBlock.blockCount + bpg - 1) / bpg;
}

static bool ext2_bgContainsBackups(sExt2 *e,u32 i) {
	/* if the sparse-feature is enabled, just the groups 0, 1 and powers of 3, 5 and 7 contain
	 * the backup */
	if(!(e->superBlock.featureRoCompat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER))
		return true;
	/* block-group 0 is handled manually */
	if(i == 1)
		return true;
	return ext2_isPowerOf(i,3) || ext2_isPowerOf(i,5) || ext2_isPowerOf(i,7);
}

static bool ext2_isPowerOf(u32 x,u32 y) {
	while(x > 1) {
		if(x % y != 0)
			return false;
		x /= y;
	}
	return true;
}
