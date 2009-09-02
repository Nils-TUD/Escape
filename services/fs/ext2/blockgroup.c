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
#include <esc/debug.h>

#include "ext2.h"
#include "blockcache.h"
#include "blockgroup.h"

#if DEBUGGING

/**
 * Prints the given bitmap
 */
static void ext2_dbg_printRanges(sExt2 *e,const char *name,u32 first,u32 max,u8 *bitmap);

void ext2_dbg_printBlockGroup(sExt2 *e,u32 no,sBlockGroup *bg) {
	debugf("	blockBitmapStart = %d\n",bg->blockBitmap);
	debugf("	inodeBitmapStart = %d\n",bg->inodeBitmap);
	debugf("	inodeTableStart = %d\n",bg->inodeTable);
	debugf("	freeBlocks = %d\n",bg->freeBlockCount);
	debugf("	freeInodes = %d\n",bg->freeInodeCount);
	debugf("	usedDirCount = %d\n",bg->usedDirCount);
	ext2_dbg_printRanges(e,"Blocks",no * e->superBlock.blocksPerGroup,
			MIN(e->superBlock.blocksPerGroup,e->superBlock.blockCount - (no * e->superBlock.blocksPerGroup)),
			ext2_bcache_request(e,bg->blockBitmap)->buffer);
	ext2_dbg_printRanges(e,"Inodes",no * e->superBlock.inodesPerGroup,
			MIN(e->superBlock.inodesPerGroup,e->superBlock.inodeCount - (no * e->superBlock.inodesPerGroup)),
			ext2_bcache_request(e,bg->inodeBitmap)->buffer);
}

static void ext2_dbg_printRanges(sExt2 *e,const char *name,u32 first,u32 max,u8 *bitmap) {
	bool lastFree;
	u32 pcount,start,i,a,j;

	pcount = 0;
	start = 0;
	lastFree = (bitmap[0] & 1) == 0;
	debugf("	Free%s:\n\t\t",name);
	for(i = 0; i < (u32)BLOCK_SIZE(e); a = 0, i++) {
		for(a = 0, j = 1; j < 256; a++, j <<= 1) {
			if(i * 8 + a >= max)
				goto freeDone;
			if(bitmap[i] & j) {
				if(lastFree) {
					if(start < i * 8 + a) {
						debugf("%d .. %d, ",first + start,first + i * 8 + a - 1);
						if(++pcount % 4 == 0)
							debugf("\n\t\t");
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
		debugf("%d .. %d, ",first + start,first + i * 8 + a - 1);
	debugf("\n");

	pcount = 0;
	start = 0;
	lastFree = (bitmap[0] & 1) == 0;
	debugf("	Used%s:\n\t\t",name);
	for(i = 0; i < (u32)BLOCK_SIZE(e); a = 0, i++) {
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
					debugf("%d .. %d, ",first + start,first + i * 8 + a - 1);
					if(++pcount % 4 == 0)
						debugf("\n\t\t");
				}
				start = i * 8 + a;
				lastFree = true;
			}
		}
	}
usedDone:
	if(!lastFree && start < i * 8 + a)
		debugf("%d .. %d, ",first + start,first + i * 8 + a - 1);
	debugf("\n");
}

#endif
