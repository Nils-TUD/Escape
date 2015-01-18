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
#include <sys/common.h>
#include <sys/debug.h>
#include <sys/endian.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bitmap.h"
#include "ext2.h"
#include "file.h"
#include "inode.h"
#include "inodecache.h"
#include "link.h"
#include "rw.h"
#include "sbmng.h"

int Ext2File::create(Ext2FileSystem *e,FSUser *u,Ext2CInode *dirNode,const char *name,ino_t *ino,mode_t mode) {
	Ext2CInode *cnode;
	int res;
	/* we need write-permission for the directory */
	if((res = e->hasPermission(dirNode,u,MODE_WRITE)) < 0)
		return res;

	res = Ext2INode::create(e,u,dirNode,&cnode,mode);
	if(res < 0)
		return res;

	/* link it to the directory */
	if((res = Ext2Link::create(e,u,dirNode,cnode,name)) < 0) {
		Ext2INode::destroy(e,cnode);
		e->inodeCache.release(cnode);
		return res;
	}

	*ino = cnode->inodeNo;
	e->inodeCache.release(cnode);
	return 0;
}

int Ext2File::remove(Ext2FileSystem *e,Ext2CInode *cnode) {
	int res;
	/* truncate the file */
	if((res = truncate(e,cnode,true)) < 0)
		return res;
	/* free inode */
	if((res = Ext2INode::destroy(e,cnode)) < 0)
		return res;
	return 0;
}

int Ext2File::truncate(Ext2FileSystem *e,Ext2CInode *cnode,bool del) {
	int res;
	size_t i;
	/* free direct blocks */
	for(i = 0; i < EXT2_DIRBLOCK_COUNT; i++) {
		if(le32tocpu(cnode->inode.dBlocks[i]) == 0)
			break;
		if((res = Ext2Bitmap::freeBlock(e,le32tocpu(cnode->inode.dBlocks[i]))) < 0)
			return res;
		if(!del)
			cnode->inode.dBlocks[i] = cputole32(0);
	}
	/* indirect */
	if(le32tocpu(cnode->inode.singlyIBlock)) {
		if((res = freeIndirBlock(e,le32tocpu(cnode->inode.singlyIBlock))) < 0)
			return res;
		if(!del)
			cnode->inode.singlyIBlock = cputole32(0);
	}
	/* double indirect */
	if(le32tocpu(cnode->inode.doublyIBlock)) {
		if((res = freeDIndirBlock(e,le32tocpu(cnode->inode.doublyIBlock))) < 0)
			return res;
		if(!del)
			cnode->inode.doublyIBlock = cputole32(0);
	}
	/* triple indirect */
	if(le32tocpu(cnode->inode.triplyIBlock)) {
		size_t count;
		CBlock *blocks = e->blockCache.request(
			le32tocpu(cnode->inode.triplyIBlock),BlockCache::WRITE);
		vassert(blocks != NULL,"Block %d set, but unable to load it\n",
				le32tocpu(cnode->inode.triplyIBlock));

		e->blockCache.markDirty(blocks);
		count = e->blockSize() / sizeof(block_t);
		for(i = 0; i < count; i++) {
			if(le32tocpu(((block_t*)blocks->buffer)[i]) == 0)
				break;
			if((res = freeDIndirBlock(e,le32tocpu(((block_t*)blocks->buffer)[i]))) < 0) {
				e->blockCache.release(blocks);
				return res;
			}
		}
		e->blockCache.release(blocks);
		if((res = Ext2Bitmap::freeBlock(e,le32tocpu(cnode->inode.triplyIBlock))) < 0)
			return res;
		if(!del)
			cnode->inode.triplyIBlock = cputole32(0);
	}

	if(!del) {
		/* reset size */
		cnode->inode.size = cputole32(0);
		cnode->inode.blocks = cputole32(0);
	}
	e->inodeCache.markDirty(cnode);
	return 0;
}

ssize_t Ext2File::read(Ext2FileSystem *e,ino_t inodeNo,void *buffer,off_t offset,size_t count) {
	Ext2CInode *cnode;
	ssize_t res;

	/* at first we need the inode */
	cnode = e->inodeCache.request(inodeNo,IMODE_WRITE);
	if(cnode == NULL)
		return -ENOBUFS;

	/* read */
	res = readIno(e,cnode,buffer,offset,count);
	if(res <= 0) {
		e->inodeCache.release(cnode);
		return res;
	}

	/* mark accessed */
	cnode->inode.accesstime = cputole32(time(NULL));
	e->inodeCache.markDirty(cnode);
	e->inodeCache.release(cnode);

	return res;
}

ssize_t Ext2File::readIno(Ext2FileSystem *e,const Ext2CInode *cnode,void *buffer,off_t offset,size_t count) {
	/* nothing left to read? */
	int32_t inoSize = le32tocpu(cnode->inode.size);
	if((int32_t)offset < 0 || (int32_t)offset >= inoSize)
		return 0;

	if(buffer != NULL) {
		size_t c,i,leftBytes,blockSize,blockCount;
		block_t startBlock;
		uint8_t *bufWork;
		/* adjust count */
		if((int32_t)(offset + count) < 0 || (int32_t)(offset + count) >= inoSize)
			count = inoSize - offset;

		blockSize = e->blockSize();
		startBlock = offset / blockSize;
		offset %= blockSize;
		blockCount = (offset + count + blockSize - 1) / blockSize;

		/* use the offset in the first block; after the first one the offset is 0 anyway */
		leftBytes = count;
		bufWork = (uint8_t*)buffer;
		for(i = 0; i < blockCount; i++) {
			/* request block */
			block_t block = Ext2INode::getDataBlock(e,cnode,startBlock + i);
			CBlock *tmpBuffer = e->blockCache.request(block,BlockCache::READ);
			if(tmpBuffer == NULL)
				return -ENOBUFS;

			/* copy the requested part */
			c = MIN(leftBytes,blockSize - offset);
			memcpy(bufWork,(uint8_t*)tmpBuffer->buffer + offset,c);
			bufWork += c;

			e->blockCache.release(tmpBuffer);

			/* we substract to much, but it matters only if we read an additional block. In this
			 * case it is correct */
			leftBytes -= blockSize - offset;
			/* offset is always 0 for additional blocks */
			offset = 0;
		}
	}
	return count;
}

ssize_t Ext2File::write(Ext2FileSystem *e,ino_t inodeNo,const void *buffer,off_t offset,size_t count) {
	/* at first we need the inode */
	Ext2CInode *cnode = e->inodeCache.request(inodeNo,IMODE_WRITE);
	if(cnode == NULL)
		return -ENOBUFS;

	/* write to it */
	count = writeIno(e,cnode,buffer,offset,count);
	e->inodeCache.release(cnode);
	return count;
}

ssize_t Ext2File::writeIno(Ext2FileSystem *e,Ext2CInode *cnode,const void *buffer,off_t offset,size_t count) {
	CBlock *tmpBuffer;
	const uint8_t *bufWork;
	time_t now;
	size_t c,i,blockSize,blockCount,leftBytes;
	block_t startBlock;
	off_t orgOff = offset;
	int32_t inoSize = le32tocpu(cnode->inode.size);

	/* gap-filling not supported yet */
	if((int32_t)offset > inoSize)
		return 0;

	blockSize = e->blockSize();
	startBlock = offset / blockSize;
	offset %= blockSize;
	blockCount = (offset + count + blockSize - 1) / blockSize;

	leftBytes = count;
	bufWork = (const uint8_t*)buffer;
	for(i = 0; i < blockCount; i++) {
		block_t block = Ext2INode::reqDataBlock(e,cnode,startBlock + i);
		/* error (e.g. no free block) ? */
		if(block == 0)
			return -ENOSPC;

		c = MIN(leftBytes,blockSize - offset);

		/* if we're not writing a complete block, we have to read it from disk first */
		if(offset != 0 || c != blockSize)
			tmpBuffer = e->blockCache.request(block,BlockCache::WRITE);
		else
			tmpBuffer = e->blockCache.create(block);
		if(tmpBuffer == NULL)
			return -ENOBUFS;
		/* we can write it to disk later :) */
		memcpy((uint8_t*)tmpBuffer->buffer + offset,bufWork,c);
		e->blockCache.markDirty(tmpBuffer);
		e->blockCache.release(tmpBuffer);

		bufWork += c;
		/* we substract to much, but it matters only if we write an additional block. In this
		 * case it is correct */
		leftBytes -= blockSize - offset;
		/* offset is always 0 for additional blocks */
		offset = 0;
	}

	/* finally, update the inode */
	now = cputole32(time(NULL));
	cnode->inode.accesstime = now;
	cnode->inode.modifytime = now;
	cnode->inode.size = (int32_t)cputole32(MAX((int32_t)(orgOff + count),inoSize));
	e->inodeCache.markDirty(cnode);
	return count;
}

int Ext2File::freeDIndirBlock(Ext2FileSystem *e,block_t blockNo) {
	size_t i,count;
	/* note that we don't need to set the block-numbers to 0 here (-> write), since the whole
	 * block is free'd afterwards anyway */
	CBlock *blocks = e->blockCache.request(blockNo,BlockCache::READ);
	vassert(blocks != NULL,"Block %d set, but unable to load it\n",blockNo);

	count = e->blockSize() / sizeof(block_t);
	for(i = 0; i < count; i++) {
		if(le32tocpu(((block_t*)blocks->buffer)[i]) == 0)
			break;
		freeIndirBlock(e,le32tocpu(((block_t*)blocks->buffer)[i]));
	}
	e->blockCache.release(blocks);
	Ext2Bitmap::freeBlock(e,blockNo);
	return 0;
}

int Ext2File::freeIndirBlock(Ext2FileSystem *e,block_t blockNo) {
	size_t i,count;
	CBlock *blocks = e->blockCache.request(blockNo,BlockCache::WRITE);
	vassert(blocks != NULL,"Block %d set, but unable to load it\n",blockNo);

	count = e->blockSize() / sizeof(block_t);
	for(i = 0; i < count; i++) {
		if(le32tocpu(((block_t*)blocks->buffer)[i]) == 0)
			break;
		Ext2Bitmap::freeBlock(e,le32tocpu(((block_t*)blocks->buffer)[i]));
		((block_t*)blocks->buffer)[i] = cputole32(0);
	}
	e->blockCache.markDirty(blocks);
	e->blockCache.release(blocks);
	Ext2Bitmap::freeBlock(e,blockNo);
	return 0;
}
