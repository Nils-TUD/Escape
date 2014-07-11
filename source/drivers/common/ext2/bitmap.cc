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
#include "sbmng.h"

ino_t Ext2Bitmap::allocInode(Ext2FileSystem *e,Ext2CInode *dirInode,bool isDir) {
	size_t gcount = e->getBlockGroupCount();
	block_t block = e->getBlockOfInode(dirInode->inodeNo);
	block_t i,group = e->getGroupOfBlock(block);
	ino_t ino = 0;
	uint32_t inodesPerGroup = le32tocpu(e->sb.get()->inodesPerGroup);

	sassert(tpool_lock(EXT2_SUPERBLOCK_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	if(le32tocpu(e->sb.get()->freeInodeCount) == 0)
		goto done;

	/* first try to find a block in the block-group of the inode */
	ino = allocInodeIn(e,group * inodesPerGroup,e->bgs.get(group),isDir);
	if(ino != 0)
		goto done;

	/* now try the other block-groups */
	for(i = group + 1; i != group; i = (i + 1) % gcount) {
		ino = allocInodeIn(e,i * inodesPerGroup,e->bgs.get(i),isDir);
		if(ino != 0)
			goto done;
	}

done:
	sassert(tpool_unlock(EXT2_SUPERBLOCK_LOCK) == 0);
	return ino;
}

int Ext2Bitmap::freeInode(Ext2FileSystem *e,ino_t ino,bool isDir) {
	block_t group = e->getGroupOfInode(ino);
	uint8_t *bitmapbuf;
	CBlock *bitmap;
	uint16_t freeInodeCount;
	uint32_t sFreeInodeCount;

	sassert(tpool_lock(EXT2_SUPERBLOCK_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	bitmap = e->blockCache.request(le32tocpu(e->bgs.get(group)->inodeBitmap),BlockCache::WRITE);
	if(bitmap == NULL) {
		sassert(tpool_unlock(EXT2_SUPERBLOCK_LOCK) == 0);
		return -1;
	}

	/* mark free in bitmap */
	ino--;
	ino %= le32tocpu(e->sb.get()->inodesPerGroup);
	bitmapbuf = (uint8_t*)bitmap->buffer;
	bitmapbuf[ino / 8] &= ~(1 << (ino % 8));

	freeInodeCount = le16tocpu(e->bgs.get(group)->freeInodeCount);
	e->bgs.get(group)->freeInodeCount = cputole16(freeInodeCount + 1);
	if(isDir) {
		uint16_t usedDirCount = le16tocpu(e->bgs.get(group)->usedDirCount);
		e->bgs.get(group)->usedDirCount = cputole16(usedDirCount - 1);
	}
	e->bgs.markDirty();

	sFreeInodeCount = le32tocpu(e->sb.get()->freeInodeCount);
	e->sb.get()->freeInodeCount = cputole32(sFreeInodeCount + 1);
	e->sb.markDirty();

	e->blockCache.markDirty(bitmap);
	e->blockCache.release(bitmap);
	sassert(tpool_unlock(EXT2_SUPERBLOCK_LOCK) == 0);
	return 0;
}

ino_t Ext2Bitmap::allocInodeIn(Ext2FileSystem *e,block_t groupStart,Ext2BlockGrp *group,bool isDir) {
	size_t i,j;
	ino_t ino;
	CBlock *bitmap;
	uint8_t *bitmapbuf;
	uint32_t sFreeInodeCount;
	if(le16tocpu(group->freeInodeCount) == 0)
		return 0;

	/* load bitmap */
	bitmap = e->blockCache.request(le32tocpu(group->inodeBitmap),BlockCache::WRITE);
	if(bitmap == NULL)
		return 0;

	ino = 0;
	bitmapbuf = (uint8_t*)bitmap->buffer;
	for(i = 0; i < e->blockSize(); i++) {
		for(j = 1; j < 256; ino++, j <<= 1) {
			if(ino >= (ino_t)le32tocpu(e->sb.get()->inodeCount)) {
				e->blockCache.release(bitmap);
				return 0;
			}
			if(!(bitmapbuf[i] & j)) {
				uint16_t freeInodeCount = le16tocpu(group->freeInodeCount);
				group->freeInodeCount = cputole16(freeInodeCount - 1);
				if(isDir) {
					uint16_t usedDirCount = le16tocpu(group->usedDirCount);
					group->usedDirCount = cputole16(usedDirCount + 1);
				}
				e->bgs.markDirty();
				bitmapbuf[i] |= j;
				sFreeInodeCount = le32tocpu(e->sb.get()->freeInodeCount);
				e->sb.get()->freeInodeCount = cputole32(sFreeInodeCount - 1);
				e->sb.markDirty();
				e->blockCache.markDirty(bitmap);
				e->blockCache.release(bitmap);
				return ino + groupStart + 1;
			}
		}
	}
	e->blockCache.release(bitmap);
	return 0;
}

block_t Ext2Bitmap::allocBlock(Ext2FileSystem *e,Ext2CInode *inode) {
	size_t gcount = e->getBlockGroupCount();
	block_t block = e->getBlockOfInode(inode->inodeNo);
	block_t i,group = e->getGroupOfBlock(block);
	block_t bno = 0;
	uint32_t blocksPerGroup = le32tocpu(e->sb.get()->blocksPerGroup);

	sassert(tpool_lock(EXT2_SUPERBLOCK_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	if(le32tocpu(e->sb.get()->freeBlockCount) == 0)
		goto done;

	/* first try to find a block in the block-group of the inode */
	bno = allocBlockIn(e,group * blocksPerGroup,e->bgs.get(group));
	if(bno != 0)
		goto done;

	/* now try the other block-groups */
	for(i = group + 1; i != group; i = (i + 1) % gcount) {
		bno = allocBlockIn(e,i * blocksPerGroup,e->bgs.get(i));
		if(bno != 0)
			goto done;
	}

done:
	sassert(tpool_unlock(EXT2_SUPERBLOCK_LOCK) == 0);
	return bno;
}

int Ext2Bitmap::freeBlock(Ext2FileSystem *e,block_t blockNo) {
	block_t group = e->getGroupOfBlock(blockNo);
	CBlock *bitmap;
	uint8_t *bitmapbuf;
	uint16_t freeBlockCount;
	uint32_t sFreeBlockCount;

	sassert(tpool_lock(EXT2_SUPERBLOCK_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);
	bitmap = e->blockCache.request(le32tocpu(e->bgs.get(group)->blockBitmap),BlockCache::WRITE);
	if(bitmap == NULL) {
		sassert(tpool_unlock(EXT2_SUPERBLOCK_LOCK) == 0);
		return -1;
	}

	/* mark free in bitmap */
	blockNo--;
	blockNo %= le32tocpu(e->sb.get()->blocksPerGroup);
	bitmapbuf = (uint8_t*)bitmap->buffer;
	bitmapbuf[blockNo / 8] &= ~(1 << (blockNo % 8));
	freeBlockCount = le16tocpu(e->bgs.get(group)->freeBlockCount);
	e->bgs.get(group)->freeBlockCount = cputole16(freeBlockCount + 1);
	e->bgs.markDirty();
	sFreeBlockCount = le32tocpu(e->sb.get()->freeBlockCount);
	e->sb.get()->freeBlockCount = cputole32(sFreeBlockCount + 1);
	e->sb.markDirty();
	e->blockCache.markDirty(bitmap);
	e->blockCache.release(bitmap);
	sassert(tpool_unlock(EXT2_SUPERBLOCK_LOCK) == 0);
	return 0;
}

block_t Ext2Bitmap::allocBlockIn(Ext2FileSystem *e,block_t groupStart,Ext2BlockGrp *group) {
	size_t i,j;
	block_t bno;
	CBlock *bitmap;
	uint8_t *bitmapbuf;
	uint32_t sFreeBlockCount;
	uint32_t blockCount = le32tocpu(e->sb.get()->blockCount);
	if(le16tocpu(group->freeBlockCount) == 0)
		return 0;

	/* load bitmap */
	bitmap = e->blockCache.request(le32tocpu(group->blockBitmap),BlockCache::WRITE);
	if(bitmap == NULL)
		return 0;

	bno = 0;
	bitmapbuf = (uint8_t*)bitmap->buffer;
	for(i = 0; i < e->blockSize(); i++) {
		for(j = 1; j < 256; bno++, j <<= 1) {
			if(bno >= blockCount) {
				e->blockCache.release(bitmap);
				return 0;
			}
			if(!(bitmapbuf[i] & j)) {
				uint16_t freeBlockCount = le16tocpu(group->freeBlockCount);
				group->freeBlockCount = cputole16(freeBlockCount - 1);
				e->bgs.markDirty();
				bitmapbuf[i] |= j;
				sFreeBlockCount = le32tocpu(e->sb.get()->freeBlockCount);
				e->sb.get()->freeBlockCount = cputole32(sFreeBlockCount - 1);
				e->sb.markDirty();
				e->blockCache.markDirty(bitmap);
				e->blockCache.release(bitmap);
				return bno + groupStart + 1;
			}
		}
	}
	e->blockCache.release(bitmap);
	return 0;
}
