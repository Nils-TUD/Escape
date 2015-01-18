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

#include <fs/blockcache.h>
#include <fs/fsdev.h>
#include <sys/common.h>
#include <sys/debug.h>
#include <sys/endian.h>
#include <sys/thread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "bgmng.h"
#include "ext2.h"
#include "rw.h"

Ext2BGMng::Ext2BGMng(Ext2FileSystem *fs) : _dirty(false), _groups(), _fs(fs) {
	/* read block-group-descriptors */
	int res;
	size_t bcount = _fs->bytesToBlocks(_fs->getBlockGroupCount());
	_groups = (Ext2BlockGrp*)malloc(bcount * _fs->blockSize());
	if(_groups == NULL)
		VTHROWE("Unable to allocate memory for blockgroups",-ENOMEM);
	if((res = Ext2RW::readBlocks(_fs,_groups,le32tocpu(_fs->sb.get()->firstDataBlock) + 1,bcount)) < 0) {
		free(_groups);
		VTHROWE("Unable to read group-table",res);
	}
}

void Ext2BGMng::update() {
	block_t bno;
	size_t i,count,bcount;
	sassert(tpool_lock(EXT2_SUPERBLOCK_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP) == 0);

	if(!_dirty)
		goto done;
	bcount = _fs->bytesToBlocks(_fs->getBlockGroupCount());

	/* update main block-group-descriptor-table */
	if(Ext2RW::writeBlocks(_fs,_groups,le32tocpu(_fs->sb.get()->firstDataBlock) + 1,bcount) != 0) {
		printe("Unable to update block-group-descriptor-table in blockgroup 0");
		goto done;
	}

	/* update block-group-descriptor backups */
	bno = le32tocpu(_fs->sb.get()->blocksPerGroup) + EXT2_BLOGRPTBL_BNO;
	count = _fs->getBlockGroupCount();
	for(i = 1; i < count; i++) {
		if(_fs->bgHasBackups(i)) {
			if(Ext2RW::writeBlocks(_fs,_groups,bno,bcount) != 0) {
				printe("Unable to update block-group-descriptor-table in blockgroup %d",i);
				goto done;
			}
		}
		bno += le32tocpu(_fs->sb.get()->blocksPerGroup);
	}

	/* now we're in sync */
	_dirty = false;
done:
	sassert(tpool_unlock(EXT2_SUPERBLOCK_LOCK) == 0);
}

#if DEBUGGING
void Ext2BGMng::print(block_t no) {
	Ext2BlockGrp *bg = get(no);
	uint32_t blocksPerGroup = le32tocpu(_fs->sb.get()->blocksPerGroup);
	uint32_t inodesPerGroup = le32tocpu(_fs->sb.get()->inodesPerGroup);
	CBlock *bbitmap = _fs->blockCache.request(le32tocpu(bg->blockBitmap),BlockCache::READ);
	CBlock *ibitmap = _fs->blockCache.request(le32tocpu(bg->inodeBitmap),BlockCache::READ);
	if(!bbitmap || !ibitmap)
		return;
	printf("	blockBitmapStart = %d\n",le32tocpu(bg->blockBitmap));
	printf("	inodeBitmapStart = %d\n",le32tocpu(bg->inodeBitmap));
	printf("	inodeTableStart = %d\n",le32tocpu(bg->inodeTable));
	printf("	freeBlocks = %d\n",le16tocpu(bg->freeBlockCount));
	printf("	freeInodes = %d\n",le16tocpu(bg->freeInodeCount));
	printf("	usedDirCount = %d\n",le16tocpu(bg->usedDirCount));
	printRanges("Blocks",no * blocksPerGroup,
			MIN(blocksPerGroup,le32tocpu(_fs->sb.get()->blockCount) - (no * blocksPerGroup)),
			static_cast<uint8_t*>(bbitmap->buffer));
	printRanges("Inodes",no * inodesPerGroup,
			MIN(inodesPerGroup,le32tocpu(_fs->sb.get()->inodeCount) - (no * inodesPerGroup)),
			static_cast<uint8_t*>(ibitmap->buffer));
	_fs->blockCache.release(ibitmap);
	_fs->blockCache.release(bbitmap);
}

void Ext2BGMng::printRanges(const char *name,block_t first,block_t max,uint8_t *bitmap) {
	bool lastFree;
	size_t pcount,start,i,a,j;

	pcount = 0;
	start = 0;
	lastFree = (bitmap[0] & 1) == 0;
	printf("	Free%s:\n\t\t",name);
	for(i = 0; i < _fs->blockSize(); a = 0, i++) {
		for(a = 0, j = 1; j < 256; a++, j <<= 1) {
			if(i * 8 + a >= max)
				goto freeDone;
			if(bitmap[i] & j) {
				if(lastFree) {
					if(start < i * 8 + a) {
						printf("%lu .. %lu, ",first + start,first + i * 8 + a - 1);
						if(++pcount % 4 == 0)
							printf("\n\t\t");
					}
					start = i * 8 + a;
					lastFree = false;
				}
			}
			else if(!lastFree) {
				start = i * 8 + a;
				lastFree = true;
			}
		}
	}
freeDone:
	if(lastFree && start < i * 8 + a)
		printf("%lu .. %lu, ",first + start,first + i * 8 + a - 1);
	printf("\n");

	pcount = 0;
	start = 0;
	lastFree = (bitmap[0] & 1) == 0;
	printf("	Used%s:\n\t\t",name);
	for(i = 0; i < _fs->blockSize(); a = 0, i++) {
		for(a = 0, j = 1; j < 256; a++, j <<= 1) {
			if(i * 8 + a >= max)
				goto usedDone;
			if((bitmap[i] & j)) {
				if(lastFree) {
					start = i * 8 + a;
					lastFree = false;
				}
			}
			else if(!lastFree) {
				if(start < i * 8 + a) {
					printf("%lu .. %lu, ",first + start,first + i * 8 + a - 1);
					if(++pcount % 4 == 0)
						printf("\n\t\t");
				}
				start = i * 8 + a;
				lastFree = true;
			}
		}
	}
usedDone:
	if(!lastFree && start < i * 8 + a)
		printf("%lu .. %lu, ",first + start,first + i * 8 + a - 1);
	printf("\n");
}
#endif
