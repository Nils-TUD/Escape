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
#include <string.h>
#include <errors.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "ext2.h"
#include "rw.h"
#include "inode.h"
#include "../blockcache.h"
#include "inodecache.h"
#include "file.h"
#include "superblock.h"
#include "link.h"
#include "bitmap.h"

/**
 * Free's the given doubly-indirect-block
 */
static s32 ext2_file_freeDIndirBlock(sExt2 *e,u32 blockNo);
/**
 * Free's the given singly-indirect-block
 */
static s32 ext2_file_freeIndirBlock(sExt2 *e,u32 blockNo);

s32 ext2_file_create(sExt2 *e,sExt2CInode *dirNode,const char *name,tInodeNo *ino,bool isDir) {
	sExt2CInode *cnode;
	s32 res = ext2_inode_create(e,dirNode,&cnode,isDir);
	if(res < 0)
		return res;

	/* link it to the directory */
	if((res = ext2_link_create(e,dirNode,cnode,name)) < 0) {
		ext2_inode_destroy(e,cnode);
		ext2_icache_release(cnode);
		return res;
	}

	*ino = cnode->inodeNo;
	ext2_icache_release(cnode);
	return 0;
}

s32 ext2_file_delete(sExt2 *e,sExt2CInode *cnode) {
	s32 res;
	/* truncate the file */
	if((res = ext2_file_truncate(e,cnode,true)) < 0)
		return res;
	/* free inode */
	if((res = ext2_inode_destroy(e,cnode)) < 0)
		return res;
	return 0;
}

s32 ext2_file_truncate(sExt2 *e,sExt2CInode *cnode,bool delete) {
	s32 res;
	u32 i;
	/* free direct blocks */
	for(i = 0; i < EXT2_DIRBLOCK_COUNT; i++) {
		if(cnode->inode.dBlocks[i] == 0)
			break;
		if((res = ext2_bm_freeBlock(e,cnode->inode.dBlocks[i])) < 0)
			return res;
		if(!delete)
			cnode->inode.dBlocks[i] = 0;
	}
	/* indirect */
	if(cnode->inode.singlyIBlock) {
		if((res = ext2_file_freeIndirBlock(e,cnode->inode.singlyIBlock)) < 0)
			return res;
		if(!delete)
			cnode->inode.singlyIBlock = 0;
	}
	/* double indirect */
	if(cnode->inode.doublyIBlock) {
		if((res = ext2_file_freeDIndirBlock(e,cnode->inode.doublyIBlock)) < 0)
			return res;
		if(!delete)
			cnode->inode.doublyIBlock = 0;
	}
	/* triple indirect */
	if(cnode->inode.triplyIBlock) {
		u32 count;
		sCBlock *blocks = bcache_request(&e->blockCache,cnode->inode.triplyIBlock,BMODE_WRITE);
		vassert(blocks != NULL,"Block %d set, but unable to load it\n",cnode->inode.triplyIBlock);

		bcache_markDirty(blocks);
		count = EXT2_BLK_SIZE(e) / sizeof(u32);
		for(i = 0; i < count; i++) {
			if(((u32*)blocks->buffer)[i] == 0)
				break;
			if((res = ext2_file_freeDIndirBlock(e,((u32*)blocks->buffer)[i])) < 0) {
				bcache_release(blocks);
				return res;
			}
		}
		bcache_release(blocks);
		if((res = ext2_bm_freeBlock(e,cnode->inode.triplyIBlock)) < 0)
			return res;
		if(!delete)
			cnode->inode.triplyIBlock = 0;
	}

	if(!delete) {
		/* reset size */
		cnode->inode.size = 0;
		cnode->inode.blocks = 0;
	}
	ext2_icache_markDirty(cnode);
	return 0;
}

s32 ext2_file_read(sExt2 *e,tInodeNo inodeNo,void *buffer,u32 offset,u32 count) {
	sExt2CInode *cnode;
	s32 res;

	/* at first we need the inode */
	cnode = ext2_icache_request(e,inodeNo,IMODE_WRITE);
	if(cnode == NULL)
		return ERR_INO_REQ_FAILED;

	/* read */
	res = ext2_file_readIno(e,cnode,buffer,offset,count);
	if(res <= 0) {
		ext2_icache_release(cnode);
		return res;
	}

	/* mark accessed */
	cnode->inode.accesstime = time(NULL);
	ext2_icache_markDirty(cnode);
	ext2_icache_release(cnode);

	return res;
}

s32 ext2_file_readIno(sExt2 *e,const sExt2CInode *cnode,void *buffer,u32 offset,u32 count) {
	/* nothing left to read? */
	if((s32)offset < 0 || (s32)offset >= cnode->inode.size)
		return 0;

	if(buffer != NULL) {
		u32 blockSize,startBlock,blockCount;
		/* adjust count */
		if((s32)(offset + count) < 0 || (s32)(offset + count) >= cnode->inode.size)
			count = cnode->inode.size - offset;

		blockSize = EXT2_BLK_SIZE(e);
		startBlock = offset / blockSize;
		offset %= blockSize;
		blockCount = (offset + count + blockSize - 1) / blockSize;

		/* use the offset in the first block; after the first one the offset is 0 anyway */
		u32 c,i,leftBytes = count;
		u8 *bufWork = (u8*)buffer;
		for(i = 0; i < blockCount; i++) {
			/* request block */
			u32 block = ext2_inode_getDataBlock(e,cnode,startBlock + i);
			sCBlock *tmpBuffer = bcache_request(&e->blockCache,block,BMODE_READ);
			if(tmpBuffer == NULL)
				return ERR_BLO_REQ_FAILED;

			/* copy the requested part */
			c = MIN(leftBytes,blockSize - offset);
			memcpy(bufWork,tmpBuffer->buffer + offset,c);
			bufWork += c;

			bcache_release(tmpBuffer);

			/* we substract to much, but it matters only if we read an additional block. In this
			 * case it is correct */
			leftBytes -= blockSize - offset;
			/* offset is always 0 for additional blocks */
			offset = 0;
		}
	}
	return count;
}

s32 ext2_file_write(sExt2 *e,tInodeNo inodeNo,const void *buffer,u32 offset,u32 count) {
	/* at first we need the inode */
	sExt2CInode *cnode = ext2_icache_request(e,inodeNo,IMODE_WRITE);
	if(cnode == NULL)
		return ERR_INO_REQ_FAILED;

	/* write to it */
	count = ext2_file_writeIno(e,cnode,buffer,offset,count);
	ext2_icache_release(cnode);
	return count;
}

s32 ext2_file_writeIno(sExt2 *e,sExt2CInode *cnode,const void *buffer,u32 offset,u32 count) {
	sCBlock *tmpBuffer;
	const u8 *bufWork;
	u32 now;
	u32 c,i,blockSize,startBlock,blockCount,leftBytes;
	u32 orgOff = offset;

	/* gap-filling not supported yet */
	if((s32)offset > cnode->inode.size)
		return 0;

	blockSize = EXT2_BLK_SIZE(e);
	startBlock = offset / blockSize;
	offset %= blockSize;
	blockCount = (offset + count + blockSize - 1) / blockSize;

	leftBytes = count;
	bufWork = (const u8*)buffer;
	for(i = 0; i < blockCount; i++) {
		u32 block = ext2_inode_reqDataBlock(e,cnode,startBlock + i);
		/* error (e.g. no free block) ? */
		if(block == 0)
			return ERR_BLO_REQ_FAILED;

		c = MIN(leftBytes,blockSize - offset);

		/* if we're not writing a complete block, we have to read it from disk first */
		if(offset != 0 || c != blockSize)
			tmpBuffer = bcache_request(&e->blockCache,block,BMODE_WRITE);
		else
			tmpBuffer = bcache_create(&e->blockCache,block);
		if(tmpBuffer == NULL)
			return 0;
		/* we can write it to disk later :) */
		memcpy(tmpBuffer->buffer + offset,bufWork,c);
		bcache_markDirty(tmpBuffer);
		bcache_release(tmpBuffer);

		bufWork += c;
		/* we substract to much, but it matters only if we write an additional block. In this
		 * case it is correct */
		leftBytes -= blockSize - offset;
		/* offset is always 0 for additional blocks */
		offset = 0;
	}

	/* finally, update the inode */
	now = time(NULL);
	cnode->inode.accesstime = now;
	cnode->inode.modifytime = now;
	cnode->inode.size = MAX((s32)(orgOff + count),cnode->inode.size);
	ext2_icache_markDirty(cnode);
	return count;
}

static s32 ext2_file_freeDIndirBlock(sExt2 *e,u32 blockNo) {
	u32 i,count;
	/* note that we don't need to set the block-numbers to 0 here (-> write), since the whole
	 * block is free'd afterwards anyway */
	sCBlock *blocks = bcache_request(&e->blockCache,blockNo,BMODE_READ);
	vassert(blocks != NULL,"Block %d set, but unable to load it\n",blockNo);

	count = EXT2_BLK_SIZE(e) / sizeof(u32);
	for(i = 0; i < count; i++) {
		if(((u32*)blocks->buffer)[i] == 0)
			break;
		ext2_file_freeIndirBlock(e,((u32*)blocks->buffer)[i]);
	}
	bcache_release(blocks);
	ext2_bm_freeBlock(e,blockNo);
	return 0;
}

static s32 ext2_file_freeIndirBlock(sExt2 *e,u32 blockNo) {
	u32 i,count;
	sCBlock *blocks = bcache_request(&e->blockCache,blockNo,BMODE_WRITE);
	vassert(blocks != NULL,"Block %d set, but unable to load it\n",blockNo);

	count = EXT2_BLK_SIZE(e) / sizeof(u32);
	for(i = 0; i < count; i++) {
		if(((u32*)blocks->buffer)[i] == 0)
			break;
		ext2_bm_freeBlock(e,((u32*)blocks->buffer)[i]);
		((u32*)blocks->buffer)[i] = 0;
	}
	bcache_markDirty(blocks);
	bcache_release(blocks);
	ext2_bm_freeBlock(e,blockNo);
	return 0;
}
