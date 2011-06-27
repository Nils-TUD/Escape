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
#include <stdlib.h>
#include <errors.h>
#include <ctype.h>
#include <string.h>
#include "file.h"
#include "iso9660.h"
#include "direcache.h"
#include "rw.h"
#include "../blockcache.h"

/**
 * Builds the required dir-entries for the fs-interface from the ISO9660-dir-entries
 */
static void iso_file_buildDirEntries(sISO9660 *h,block_t lba,uint8_t *dst,const uint8_t *src,
		off_t offset,size_t count);

ssize_t iso_file_read(sISO9660 *h,inode_t inodeNo,void *buffer,off_t offset,size_t count) {
	const sISOCDirEntry *e;
	sCBlock *blk;
	uint8_t *bufWork;
	block_t startBlock;
	size_t c,i,blockSize,blockCount,leftBytes;

	/* at first we need the direntry */
	e = iso_direc_get(h,inodeNo);
	if(e == NULL)
		return ERR_INO_REQ_FAILED;

	/* nothing left to read? */
	if(offset >= e->entry.extentSize.littleEndian)
		return 0;
	/* adjust count */
	if((offset + count) >= e->entry.extentSize.littleEndian)
		count = e->entry.extentSize.littleEndian - offset;

	blockSize = ISO_BLK_SIZE(h);
	startBlock = e->entry.extentLoc.littleEndian + offset / blockSize;
	offset %= blockSize;
	blockCount = (offset + count + blockSize - 1) / blockSize;

	/* TODO try to read multiple blocks at once */

	/* use the offset in the first block; after the first one the offset is 0 anyway */
	leftBytes = count;
	bufWork = (uint8_t*)buffer;
	for(i = 0; i < blockCount; i++) {
		/* read block */
		blk = bcache_request(&h->blockCache,startBlock + i,BMODE_READ);
		if(blk == NULL)
			return ERR_BLO_REQ_FAILED;

		if(buffer != NULL) {
			/* copy the requested part */
			c = MIN(leftBytes,blockSize - offset);
			if(e->entry.flags & ISO_FILEFL_DIR)
				iso_file_buildDirEntries(h,e->entry.extentLoc.littleEndian,bufWork,
						blk->buffer,offset,c);
			else
				memcpy(bufWork,(void*)((uintptr_t)blk->buffer + offset),c);
			bufWork += c;
		}
		bcache_release(blk);

		/* we substract to much, but it matters only if we read an additional block. In this
		 * case it is correct */
		leftBytes -= blockSize - offset;
		/* offset is always 0 for additional blocks */
		offset = 0;
	}

	return count;
}

static void iso_file_buildDirEntries(sISO9660 *h,block_t lba,uint8_t *dst,const uint8_t *src,
		off_t offset,size_t count) {
	const sISODirEntry *e;
	sDirEntry *de,*lastDe;
	size_t i,blockSize = ISO_BLK_SIZE(h);
	uint8_t *cdst;

	/* TODO the whole stuff here is of course not a good solution. but it works, thats enough
	 * for now :) */

	if(offset != 0 || count != blockSize)
		cdst = (uint8_t*)malloc(blockSize);
	else
		cdst = dst;

	e = (const sISODirEntry*)src;
	lastDe = NULL;
	de = (sDirEntry*)cdst;
	while((uintptr_t)e < (uintptr_t)src + blockSize) {
		if(e->length == 0) {
			/* stretch last entry till the block-boundary and clear the rest */
			if(lastDe)
				lastDe->recLen += ((uintptr_t)cdst + blockSize) - (uintptr_t)de;
			memclear(de,((uintptr_t)cdst + blockSize) - (uintptr_t)de);
			break;
		}
		de->nodeNo = (lba * blockSize) + ((uintptr_t)e - (uintptr_t)src);
		if(e->name[0] == ISO_FILENAME_THIS) {
			de->nameLen = 1;
			de->name[0] = '.';
		}
		else if(e->name[0] == ISO_FILENAME_PARENT) {
			de->nameLen = 2;
			memcpy(de->name,"..",2);
		}
		else {
			de->nameLen = MIN(e->nameLen,strchri(e->name,';'));
			for(i = 0; i < de->nameLen; i++)
				de->name[i] = tolower(e->name[i]);
		}
		de->recLen = (sizeof(sDirEntry) - (MAX_NAME_LEN + 1)) + de->nameLen;
		lastDe = de;
		de = (sDirEntry*)((uintptr_t)de + de->recLen);
		e = (const sISODirEntry*)((uintptr_t)e + e->length);
	}

	if(offset != 0 || count != blockSize) {
		memcpy(dst,cdst + offset,count);
		free(cdst);
	}
}
