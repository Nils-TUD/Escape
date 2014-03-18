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
#include <esc/thread.h>
#include <esc/endian.h>
#include <fs/blockcache.h>
#include <fs/fsdev.h>
#include <assert.h>

#include "ext2.h"
#include "bitmap.h"
#include "superblock.h"

/**
 * Allocates an inode in the given block-group
 */
static inode_t ext2_bm_allocInodeIn(sExt2 *e,block_t groupStart,sExt2BlockGrp *group,bool isDir);
/**
 * Allocates a block in the given block-group
 */
static block_t ext2_bm_allocBlockIn(sExt2 *e,block_t groupStart,sExt2BlockGrp *group);

inode_t ext2_bm_allocInode(sExt2 *e,sExt2CInode *dirInode,bool isDir) {
	size_t gcount = ext2_getBlockGroupCount(e);
	block_t block = ext2_getBlockOfInode(e,dirInode->inodeNo);
	block_t i,group = ext2_getGroupOfBlock(e,block);
	inode_t ino = 0;
	uint32_t inodesPerGroup = le32tocpu(e->superBlock.inodesPerGroup);

	assert(tpool_lock(EXT2_SUPERBLOCK_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	if(le32tocpu(e->superBlock.freeInodeCount) == 0)
		goto done;

	/* first try to find a block in the block-group of the inode */
	ino = ext2_bm_allocInodeIn(e,group * inodesPerGroup,e->groups + group,isDir);
	if(ino != 0)
		goto done;

	/* now try the other block-groups */
	for(i = group + 1; i != group; i = (i + 1) % gcount) {
		ino = ext2_bm_allocInodeIn(e,i * inodesPerGroup,e->groups + i,isDir);
		if(ino != 0)
			goto done;
	}

done:
	assert(tpool_unlock(EXT2_SUPERBLOCK_LOCK) == 0);
	return ino;
}

int ext2_bm_freeInode(sExt2 *e,inode_t ino,bool isDir) {
	block_t group = ext2_getGroupOfInode(e,ino);
	uint8_t *bitmapbuf;
	sCBlock *bitmap;
	uint16_t freeInodeCount;
	uint32_t sFreeInodeCount;

	assert(tpool_lock(EXT2_SUPERBLOCK_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	bitmap = bcache_request(&e->blockCache,le32tocpu(e->groups[group].inodeBitmap),BMODE_WRITE);
	if(bitmap == NULL) {
		assert(tpool_unlock(EXT2_SUPERBLOCK_LOCK) == 0);
		return -1;
	}

	/* mark free in bitmap */
	ino--;
	ino %= le32tocpu(e->superBlock.inodesPerGroup);
	bitmapbuf = (uint8_t*)bitmap->buffer;
	bitmapbuf[ino / 8] &= ~(1 << (ino % 8));
	freeInodeCount = le16tocpu(e->groups[group].freeInodeCount);
	e->groups[group].freeInodeCount = cputole16(freeInodeCount + 1);
	if(isDir) {
		uint16_t usedDirCount = le16tocpu(e->groups[group].usedDirCount);
		e->groups[group].usedDirCount = cputole16(usedDirCount - 1);
	}
	e->groupsDirty = true;
	sFreeInodeCount = le32tocpu(e->superBlock.freeInodeCount);
	e->superBlock.freeInodeCount = cputole32(sFreeInodeCount + 1);
	e->sbDirty = true;
	bcache_markDirty(bitmap);
	bcache_release(bitmap);
	assert(tpool_unlock(EXT2_SUPERBLOCK_LOCK) == 0);
	return 0;
}

static inode_t ext2_bm_allocInodeIn(sExt2 *e,block_t groupStart,sExt2BlockGrp *group,bool isDir) {
	size_t i,j;
	inode_t ino;
	sCBlock *bitmap;
	uint8_t *bitmapbuf;
	uint32_t sFreeInodeCount;
	if(le16tocpu(group->freeInodeCount) == 0)
		return 0;

	/* load bitmap */
	bitmap = bcache_request(&e->blockCache,le32tocpu(group->inodeBitmap),BMODE_WRITE);
	if(bitmap == NULL)
		return 0;

	ino = 0;
	bitmapbuf = (uint8_t*)bitmap->buffer;
	for(i = 0; i < EXT2_BLK_SIZE(e); i++) {
		for(j = 1; j < 256; ino++, j <<= 1) {
			if(ino >= (inode_t)le32tocpu(e->superBlock.inodeCount)) {
				bcache_release(bitmap);
				return 0;
			}
			if(!(bitmapbuf[i] & j)) {
				uint16_t freeInodeCount = le16tocpu(group->freeInodeCount);
				group->freeInodeCount = cputole16(freeInodeCount - 1);
				if(isDir) {
					uint16_t usedDirCount = le16tocpu(group->usedDirCount);
					group->usedDirCount = cputole16(usedDirCount + 1);
				}
				e->groupsDirty = true;
				bitmapbuf[i] |= j;
				sFreeInodeCount = le32tocpu(e->superBlock.freeInodeCount);
				e->superBlock.freeInodeCount = cputole32(sFreeInodeCount - 1);
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

block_t ext2_bm_allocBlock(sExt2 *e,sExt2CInode *inode) {
	size_t gcount = ext2_getBlockGroupCount(e);
	block_t block = ext2_getBlockOfInode(e,inode->inodeNo);
	block_t i,group = ext2_getGroupOfBlock(e,block);
	block_t bno = 0;
	uint32_t blocksPerGroup = le32tocpu(e->superBlock.blocksPerGroup);

	assert(tpool_lock(EXT2_SUPERBLOCK_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	if(le32tocpu(e->superBlock.freeBlockCount) == 0)
		goto done;

	/* first try to find a block in the block-group of the inode */
	bno = ext2_bm_allocBlockIn(e,group * blocksPerGroup,e->groups + group);
	if(bno != 0)
		goto done;

	/* now try the other block-groups */
	for(i = group + 1; i != group; i = (i + 1) % gcount) {
		bno = ext2_bm_allocBlockIn(e,i * blocksPerGroup,e->groups + i);
		if(bno != 0)
			goto done;
	}

done:
	assert(tpool_unlock(EXT2_SUPERBLOCK_LOCK) == 0);
	return bno;
}

int ext2_bm_freeBlock(sExt2 *e,block_t blockNo) {
	block_t group = ext2_getGroupOfBlock(e,blockNo);
	sCBlock *bitmap;
	uint8_t *bitmapbuf;
	uint16_t freeBlockCount;
	uint32_t sFreeBlockCount;

	assert(tpool_lock(EXT2_SUPERBLOCK_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	bitmap = bcache_request(&e->blockCache,le32tocpu(e->groups[group].blockBitmap),BMODE_WRITE);
	if(bitmap == NULL) {
		assert(tpool_unlock(EXT2_SUPERBLOCK_LOCK) == 0);
		return -1;
	}

	/* mark free in bitmap */
	blockNo--;
	blockNo %= le32tocpu(e->superBlock.blocksPerGroup);
	bitmapbuf = (uint8_t*)bitmap->buffer;
	bitmapbuf[blockNo / 8] &= ~(1 << (blockNo % 8));
	freeBlockCount = le16tocpu(e->groups[group].freeBlockCount);
	e->groups[group].freeBlockCount = cputole16(freeBlockCount + 1);
	e->groupsDirty = true;
	sFreeBlockCount = le32tocpu(e->superBlock.freeBlockCount);
	e->superBlock.freeBlockCount = cputole32(sFreeBlockCount + 1);
	e->sbDirty = true;
	bcache_markDirty(bitmap);
	bcache_release(bitmap);
	assert(tpool_unlock(EXT2_SUPERBLOCK_LOCK) == 0);
	return 0;
}

static block_t ext2_bm_allocBlockIn(sExt2 *e,block_t groupStart,sExt2BlockGrp *group) {
	size_t i,j;
	block_t bno;
	sCBlock *bitmap;
	uint8_t *bitmapbuf;
	uint32_t sFreeBlockCount;
	uint32_t blockCount = le32tocpu(e->superBlock.blockCount);
	if(le16tocpu(group->freeBlockCount) == 0)
		return 0;

	/* load bitmap */
	bitmap = bcache_request(&e->blockCache,le32tocpu(group->blockBitmap),BMODE_WRITE);
	if(bitmap == NULL)
		return 0;

	bno = 0;
	bitmapbuf = (uint8_t*)bitmap->buffer;
	for(i = 0; i < EXT2_BLK_SIZE(e); i++) {
		for(j = 1; j < 256; bno++, j <<= 1) {
			if(bno >= blockCount) {
				bcache_release(bitmap);
				return 0;
			}
			if(!(bitmapbuf[i] & j)) {
				uint16_t freeBlockCount = le16tocpu(group->freeBlockCount);
				group->freeBlockCount = cputole16(freeBlockCount - 1);
				e->groupsDirty = true;
				bitmapbuf[i] |= j;
				sFreeBlockCount = le32tocpu(e->superBlock.freeBlockCount);
				e->superBlock.freeBlockCount = cputole32(sFreeBlockCount - 1);
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
