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
#include <esc/thread.h>
#include <assert.h>
#include "ext2.h"
#include "bitmap.h"
#include "superblock.h"
#include "../blockcache.h"

/**
 * Allocates an inode in the given block-group
 */
static tInodeNo ext2_bm_allocInodeIn(sExt2 *e,tBlockNo groupStart,sExt2BlockGrp *group,bool isDir);
/**
 * Allocates a block in the given block-group
 */
static tBlockNo ext2_bm_allocBlockIn(sExt2 *e,tBlockNo groupStart,sExt2BlockGrp *group);

tInodeNo ext2_bm_allocInode(sExt2 *e,sExt2CInode *dirInode,bool isDir) {
	size_t gcount = ext2_getBlockGroupCount(e);
	tBlockNo block = ext2_getBlockOfInode(e,dirInode->inodeNo);
	tBlockNo i,group = ext2_getGroupOfBlock(e,block);
	tInodeNo ino = 0;

	assert(lock(EXT2_SUPERBLOCK_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	if(e->superBlock.freeInodeCount == 0)
		goto done;

	/* first try to find a block in the block-group of the inode */
	ino = ext2_bm_allocInodeIn(e,group * e->superBlock.inodesPerGroup,e->groups + group,isDir);
	if(ino != 0)
		goto done;

	/* now try the other block-groups */
	for(i = group + 1; i != group; i = (i + 1) % gcount) {
		ino = ext2_bm_allocInodeIn(e,i * e->superBlock.inodesPerGroup,e->groups + i,isDir);
		if(ino != 0)
			goto done;
	}

done:
	assert(unlock(EXT2_SUPERBLOCK_LOCK) == 0);
	return ino;
}

int ext2_bm_freeInode(sExt2 *e,tInodeNo ino,bool isDir) {
	tBlockNo group = ext2_getGroupOfInode(e,ino);
	uint8_t *bitmapbuf;
	sCBlock *bitmap;

	assert(lock(EXT2_SUPERBLOCK_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	bitmap = bcache_request(&e->blockCache,e->groups[group].inodeBitmap,BMODE_WRITE);
	if(bitmap == NULL) {
		assert(unlock(EXT2_SUPERBLOCK_LOCK) == 0);
		return -1;
	}

	/* mark free in bitmap */
	ino--;
	ino %= e->superBlock.inodesPerGroup;
	bitmapbuf = (uint8_t*)bitmap->buffer;
	bitmapbuf[ino / 8] &= ~(1 << (ino % 8));
	e->groups[group].freeInodeCount++;
	if(isDir)
		e->groups[group].usedDirCount--;
	e->groupsDirty = true;
	e->superBlock.freeInodeCount++;
	e->sbDirty = true;
	bcache_markDirty(bitmap);
	bcache_release(bitmap);
	assert(unlock(EXT2_SUPERBLOCK_LOCK) == 0);
	return 0;
}

static tInodeNo ext2_bm_allocInodeIn(sExt2 *e,tBlockNo groupStart,sExt2BlockGrp *group,bool isDir) {
	size_t i,j;
	tInodeNo ino;
	sCBlock *bitmap;
	uint8_t *bitmapbuf;
	if(group->freeInodeCount == 0)
		return 0;

	/* load bitmap */
	bitmap = bcache_request(&e->blockCache,group->inodeBitmap,BMODE_WRITE);
	if(bitmap == NULL)
		return 0;

	ino = 0;
	bitmapbuf = (uint8_t*)bitmap->buffer;
	for(i = 0; i < EXT2_BLK_SIZE(e); i++) {
		for(j = 1; j < 256; ino++, j <<= 1) {
			if(ino >= (tInodeNo)e->superBlock.inodeCount) {
				bcache_release(bitmap);
				return 0;
			}
			if(!(bitmapbuf[i] & j)) {
				group->freeInodeCount--;
				if(isDir)
					group->usedDirCount++;
				e->groupsDirty = true;
				bitmapbuf[i] |= j;
				e->superBlock.freeInodeCount--;
				e->sbDirty = true;
				bcache_markDirty(bitmap);
				bcache_release(bitmap);
				return ino + groupStart + 1;
			}
		}
	}
	bcache_release(bitmap);
	return 0;
}

tBlockNo ext2_bm_allocBlock(sExt2 *e,sExt2CInode *inode) {
	size_t gcount = ext2_getBlockGroupCount(e);
	tBlockNo block = ext2_getBlockOfInode(e,inode->inodeNo);
	tBlockNo i,group = ext2_getGroupOfBlock(e,block);
	tBlockNo bno = 0;

	assert(lock(EXT2_SUPERBLOCK_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	if(e->superBlock.freeBlockCount == 0)
		goto done;

	/* first try to find a block in the block-group of the inode */
	bno = ext2_bm_allocBlockIn(e,group * e->superBlock.blocksPerGroup,e->groups + group);
	if(bno != 0)
		goto done;

	/* now try the other block-groups */
	for(i = group + 1; i != group; i = (i + 1) % gcount) {
		bno = ext2_bm_allocBlockIn(e,i * e->superBlock.blocksPerGroup,e->groups + i);
		if(bno != 0)
			goto done;
	}

done:
	assert(unlock(EXT2_SUPERBLOCK_LOCK) == 0);
	return bno;
}

int ext2_bm_freeBlock(sExt2 *e,tBlockNo blockNo) {
	tBlockNo group = ext2_getGroupOfBlock(e,blockNo);
	sCBlock *bitmap;
	uint8_t *bitmapbuf;

	assert(lock(EXT2_SUPERBLOCK_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	bitmap = bcache_request(&e->blockCache,e->groups[group].blockBitmap,BMODE_WRITE);
	if(bitmap == NULL) {
		assert(unlock(EXT2_SUPERBLOCK_LOCK) == 0);
		return -1;
	}

	/* mark free in bitmap */
	blockNo--;
	blockNo %= e->superBlock.blocksPerGroup;
	bitmapbuf = (uint8_t*)bitmap->buffer;
	bitmapbuf[blockNo / 8] &= ~(1 << (blockNo % 8));
	e->groups[group].freeBlockCount++;
	e->groupsDirty = true;
	e->superBlock.freeBlockCount++;
	e->sbDirty = true;
	bcache_markDirty(bitmap);
	bcache_release(bitmap);
	assert(unlock(EXT2_SUPERBLOCK_LOCK) == 0);
	return 0;
}

static tBlockNo ext2_bm_allocBlockIn(sExt2 *e,tBlockNo groupStart,sExt2BlockGrp *group) {
	size_t i,j;
	tBlockNo bno;
	sCBlock *bitmap;
	uint8_t *bitmapbuf;
	if(group->freeBlockCount == 0)
		return 0;

	/* load bitmap */
	bitmap = bcache_request(&e->blockCache,group->blockBitmap,BMODE_WRITE);
	if(bitmap == NULL)
		return 0;

	bno = 0;
	bitmapbuf = (uint8_t*)bitmap->buffer;
	for(i = 0; i < EXT2_BLK_SIZE(e); i++) {
		for(j = 1; j < 256; bno++, j <<= 1) {
			if(bno >= e->superBlock.blockCount) {
				bcache_release(bitmap);
				return 0;
			}
			if(!(bitmapbuf[i] & j)) {
				group->freeBlockCount--;
				e->groupsDirty = true;
				bitmapbuf[i] |= j;
				e->superBlock.freeBlockCount--;
				e->sbDirty = true;
				bcache_markDirty(bitmap);
				bcache_release(bitmap);
				return bno + groupStart + 1;
			}
		}
	}
	bcache_release(bitmap);
	return 0;
}
