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
#include "ext2.h"
#include "bitmap.h"
#include "superblock.h"
#include "blockcache.h"

/**
 * Allocates an inode in the given block-group
 */
static tInodeNo ext2_bm_allocInodeIn(sExt2 *e,u32 groupStart,sExt2BlockGrp *group,bool isDir);
/**
 * Allocates a block in the given block-group
 */
static u32 ext2_bm_allocBlockIn(sExt2 *e,u32 groupStart,sExt2BlockGrp *group);

tInodeNo ext2_bm_allocInode(sExt2 *e,sExt2CInode *dirInode,bool isDir) {
	u32 gcount = ext2_getBlockGroupCount(e);
	u32 block = ext2_getBlockOfInode(e,dirInode->inodeNo);
	u32 group = ext2_getGroupOfBlock(e,block);
	tInodeNo ino;
	u32 i;

	if(e->superBlock.freeInodeCount == 0)
		return 0;

	/* first try to find a block in the block-group of the inode */
	ino = ext2_bm_allocInodeIn(e,group * e->superBlock.inodesPerGroup,e->groups + group,isDir);
	if(ino != 0)
		return ino;

	/* now try the other block-groups */
	for(i = group + 1; i != group; i = (i + 1) % gcount) {
		ino = ext2_bm_allocInodeIn(e,i * e->superBlock.inodesPerGroup,e->groups + i,isDir);
		if(ino != 0)
			return ino;
	}
	return 0;
}

s32 ext2_bm_freeInode(sExt2 *e,tInodeNo ino,bool isDir) {
	u32 group = ext2_getGroupOfInode(e,ino);
	sExt2CBlock *bitmap = ext2_bcache_request(e,e->groups[group].inodeBitmap);
	if(bitmap == NULL)
		return -1;

	/* mark free in bitmap */
	ino--;
	ino %= e->superBlock.inodesPerGroup;
	bitmap->buffer[ino / 8] &= ~(1 << (ino % 8));
	bitmap->dirty = true;
	e->groups[group].freeInodeCount++;
	if(isDir)
		e->groups[group].usedDirCount--;
	e->groupsDirty = true;
	e->superBlock.freeInodeCount++;
	e->sbDirty = true;
	return 0;
}

static tInodeNo ext2_bm_allocInodeIn(sExt2 *e,u32 groupStart,sExt2BlockGrp *group,bool isDir) {
	u32 i,j;
	tInodeNo ino;
	sExt2CBlock *bitmap;
	if(group->freeInodeCount == 0)
		return 0;

	/* load bitmap */
	bitmap = ext2_bcache_request(e,group->inodeBitmap);
	if(bitmap == NULL)
		return 0;

	ino = 0;
	for(i = 0; i < (u32)EXT2_BLK_SIZE(e); i++) {
		for(j = 1; j < 256; ino++, j <<= 1) {
			if(ino >= (s32)e->superBlock.inodeCount)
				return 0;
			if(!(bitmap->buffer[i] & j)) {
				group->freeInodeCount--;
				if(isDir)
					group->usedDirCount++;
				e->groupsDirty = true;
				bitmap->buffer[i] |= j;
				bitmap->dirty = true;
				e->superBlock.freeInodeCount--;
				e->sbDirty = true;
				return ino + groupStart + 1;
			}
		}
	}
	return 0;
}

u32 ext2_bm_allocBlock(sExt2 *e,sExt2CInode *inode) {
	u32 gcount = ext2_getBlockGroupCount(e);
	u32 block = ext2_getBlockOfInode(e,inode->inodeNo);
	u32 group = ext2_getGroupOfBlock(e,block);
	u32 i,bno;

	if(e->superBlock.freeBlockCount == 0)
		return 0;

	/* first try to find a block in the block-group of the inode */
	bno = ext2_bm_allocBlockIn(e,group * e->superBlock.blocksPerGroup,e->groups + group);
	if(bno != 0)
		return bno;

	/* now try the other block-groups */
	for(i = group + 1; i != group; i = (i + 1) % gcount) {
		bno = ext2_bm_allocBlockIn(e,i * e->superBlock.blocksPerGroup,e->groups + i);
		if(bno != 0)
			return bno;
	}
	return 0;
}

s32 ext2_bm_freeBlock(sExt2 *e,u32 blockNo) {
	u32 group = ext2_getGroupOfBlock(e,blockNo);
	sExt2CBlock *bitmap = ext2_bcache_request(e,e->groups[group].blockBitmap);
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

static u32 ext2_bm_allocBlockIn(sExt2 *e,u32 groupStart,sExt2BlockGrp *group) {
	u32 i,j,bno;
	sExt2CBlock *bitmap;
	if(group->freeBlockCount == 0)
		return 0;

	/* load bitmap */
	bitmap = ext2_bcache_request(e,group->blockBitmap);
	if(bitmap == NULL)
		return 0;

	bno = 0;
	for(i = 0; i < (u32)EXT2_BLK_SIZE(e); i++) {
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
