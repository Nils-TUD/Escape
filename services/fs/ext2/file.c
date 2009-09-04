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
#include <esc/debug.h>
#include <esc/date.h>
#include <string.h>
#include <errors.h>

#include "ext2.h"
#include "request.h"
#include "inode.h"
#include "blockcache.h"
#include "inodecache.h"
#include "file.h"
#include "superblock.h"
#include "link.h"

sCachedInode *ext2_createFile(sExt2 *e,sCachedInode *dirNode,const char *name) {
	u32 i,now;
	sCachedInode *cnode;

	/* request inode */
	tInodeNo ino = ext2_allocInode(e,dirNode);
	if(ino == 0)
		return NULL;
	cnode = ext2_icache_request(e,ino);
	if(cnode == NULL) {
		ext2_freeInode(e,ino);
		return NULL;
	}

	/* init inode */
	cnode->inode.gid = 0;
	cnode->inode.uid = 0;
	cnode->inode.mode = EXT2_S_IFREG | EXT2_S_IRUSR | EXT2_S_IWUSR | EXT2_S_IRGRP | EXT2_S_IROTH;
	cnode->inode.linkCount = 0;
	cnode->inode.size = 0;
	cnode->inode.singlyIBlock = 0;
	cnode->inode.doublyIBlock = 0;
	cnode->inode.triplyIBlock = 0;
	for(i = 0; i < EXT2_DIRBLOCK_COUNT; i++)
		cnode->inode.dBlocks[i] = 0;
	cnode->inode.blocks = 0;
	now = getTime();
	cnode->inode.accesstime = now;
	cnode->inode.createtime = now;
	cnode->inode.modifytime = now;

	/* link it to the directory */
	if(ext2_link(e,dirNode,cnode,name) != 0) {
		ext2_icache_release(e,cnode);
		ext2_freeInode(e,ino);
		return NULL;
	}

	cnode->dirty = true;
	return cnode;
}

s32 ext2_readFile(sExt2 *e,tInodeNo inodeNo,void *buffer,u32 offset,u32 count) {
	sCachedInode *cnode;
	sBCacheEntry *tmpBuffer;
	u8 *bufWork;
	u32 c,i,blockSize,startBlock,blockCount,leftBytes;

	/* at first we need the inode */
	cnode = ext2_icache_request(e,inodeNo);
	if(cnode == NULL)
		return ERR_FS_READ_FAILED;

	/* nothing left to read? */
	if((s32)offset < 0 || (s32)offset >= cnode->inode.size)
		return 0;
	/* adjust count */
	if((s32)(offset + count) < 0 || (s32)(offset + count) >= cnode->inode.size)
		count = cnode->inode.size - offset;

	blockSize = BLOCK_SIZE(e);
	startBlock = offset / blockSize;
	offset %= blockSize;
	blockCount = (offset + count + blockSize - 1) / blockSize;

	/* TODO try to read multiple blocks at once */

	/* use the offset in the first block; after the first one the offset is 0 anyway */
	leftBytes = count;
	bufWork = (u8*)buffer;
	for(i = 0; i < blockCount; i++) {
		u32 block = ext2_getDataBlock(e,cnode,startBlock + i);

		/* request block */
		tmpBuffer = ext2_bcache_request(e,block);
		if(tmpBuffer == NULL)
			return ERR_FS_READ_FAILED;

		if(buffer != NULL) {
			/* copy the requested part */
			c = MIN(leftBytes,blockSize - offset);
			memcpy(bufWork,tmpBuffer->buffer + offset,c);
			bufWork += c;
		}

		/* we substract to much, but it matters only if we read an additional block. In this
		 * case it is correct */
		leftBytes -= blockSize - offset;
		/* offset is always 0 for additional blocks */
		offset = 0;
	}

	return count;
}

s32 ext2_writeFile(sExt2 *e,tInodeNo inodeNo,const void *buffer,u32 offset,u32 count) {
	sCachedInode *cnode;
	sBCacheEntry *tmpBuffer;
	const u8 *bufWork;
	u32 c,i,blockSize,startBlock,blockCount,leftBytes;
	u32 orgOff = offset;

	/* at first we need the inode */
	cnode = ext2_icache_request(e,inodeNo);
	if(cnode == NULL)
		return ERR_FS_READ_FAILED;

	/* gap-filling not supported yet */
	if((s32)offset > cnode->inode.size)
		return 0;

	blockSize = BLOCK_SIZE(e);
	startBlock = offset / blockSize;
	offset %= blockSize;
	blockCount = (offset + count + blockSize - 1) / blockSize;

	leftBytes = count;
	bufWork = (const u8*)buffer;
	for(i = 0; i < blockCount; i++) {
		u32 block = ext2_getDataBlock(e,cnode,startBlock + i);
		c = MIN(leftBytes,blockSize - offset);

		/* if we're not writing a complete block, we have to read it from disk first */
		if(offset != 0 || c != blockSize)
			tmpBuffer = ext2_bcache_request(e,block);
		else
			tmpBuffer = ext2_bcache_create(e,block);
		if(tmpBuffer == NULL)
			return 0;
		/* we can write it later to disk :) */
		memcpy(tmpBuffer->buffer + offset,bufWork,c);
		tmpBuffer->dirty = true;

		bufWork += c;
		/* we substract to much, but it matters only if we write an additional block. In this
		 * case it is correct */
		leftBytes -= blockSize - offset;
		/* offset is always 0 for additional blocks */
		offset = 0;
	}

	/* finally, update the inode */
	cnode->inode.modifytime = getTime();
	cnode->inode.size = MAX((s32)(orgOff + count),cnode->inode.size);
	cnode->dirty = true;

	return count;
}
