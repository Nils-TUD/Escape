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
#include <errors.h>
#include <string.h>
#include "file.h"
#include "iso9660.h"
#include "direcache.h"
#include "rw.h"

/**
 * Builds the required dir-entries for the fs-interface from the ISO9660-dir-entries
 */
static void iso_file_buildDirEntries(sISO9660 *h,u32 lba,u8 *dst,u8 *src,u32 offset,u32 count);

s32 iso_file_read(sISO9660 *h,tInodeNo inodeNo,void *buffer,u32 offset,u32 count) {
	sISOCDirEntry *e;
	u8 *tmpBuffer;
	u8 *bufWork;
	u32 c,i,blockSize,startBlock,blockCount,leftBytes;

	/* at first we need the direntry */
	e = iso_direc_get(h,inodeNo);
	if(e == NULL)
		return ERR_INO_REQ_FAILED;

	/* nothing left to read? */
	if((s32)offset < 0 || offset >= e->entry.extentSize.littleEndian)
		return 0;
	/* adjust count */
	if((s32)(offset + count) < 0 || (offset + count) >= e->entry.extentSize.littleEndian)
		count = e->entry.extentSize.littleEndian - offset;

	blockSize = ISO_BLK_SIZE(h);
	startBlock = e->entry.extentLoc.littleEndian + offset / blockSize;
	offset %= blockSize;
	blockCount = (offset + count + blockSize - 1) / blockSize;

	tmpBuffer = (u8*)malloc(blockSize);
	if(tmpBuffer == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* TODO try to read multiple blocks at once */

	/* use the offset in the first block; after the first one the offset is 0 anyway */
	leftBytes = count;
	bufWork = (u8*)buffer;
	for(i = 0; i < blockCount; i++) {
		/* read block */
		if(!iso_rw_readBlocks(h,tmpBuffer,startBlock + i,1)) {
			free(tmpBuffer);
			return ERR_BLO_REQ_FAILED;
		}

		if(buffer != NULL) {
			/* copy the requested part */
			c = MIN(leftBytes,blockSize - offset);
			if(e->entry.flags & ISO_FILEFL_DIR)
				iso_file_buildDirEntries(h,e->entry.extentLoc.littleEndian,bufWork,tmpBuffer,offset,c);
			else
				memcpy(bufWork,tmpBuffer + offset,c);
			bufWork += c;
		}

		/* we substract to much, but it matters only if we read an additional block. In this
		 * case it is correct */
		leftBytes -= blockSize - offset;
		/* offset is always 0 for additional blocks */
		offset = 0;
	}

	free(tmpBuffer);
	return count;
}

static void iso_file_buildDirEntries(sISO9660 *h,u32 lba,u8 *dst,u8 *src,u32 offset,u32 count) {
	sISODirEntry *e;
	sDirEntry *de,*lastDe;
	u32 blockSize = ISO_BLK_SIZE(h);
	u8 *cdst;

	/* TODO the whole stuff here is of course not a good solution. but it works, thats enough
	 * for now :) */

	if(offset != 0 || count != blockSize)
		cdst = (u8*)malloc(blockSize);
	else
		cdst = dst;

	e = (sISODirEntry*)src;
	lastDe = NULL;
	de = (sDirEntry*)cdst;
	while((u8*)e < (u8*)src + blockSize) {
		if(e->length == 0) {
			/* stretch last entry till the block-boundary and clear the rest */
			if(lastDe)
				lastDe->recLen += ((u8*)cdst + blockSize) - (u8*)de;
			memclear(de,((u8*)cdst + blockSize) - (u8*)de);
			break;
		}
		de->nodeNo = (lba * blockSize) + ((u8*)e - (u8*)src);
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
			memcpy(de->name,e->name,de->nameLen);
		}
		de->recLen = (sizeof(sDirEntry) - (MAX_NAME_LEN + 1)) + de->nameLen;
		lastDe = de;
		de = (sDirEntry*)((u8*)de + de->recLen);
		e = (sISODirEntry*)((u8*)e + e->length);
	}

	if(offset != 0 || count != blockSize) {
		memcpy(dst,cdst + offset,count);
		free(cdst);
	}
}
