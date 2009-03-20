/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <heap.h>
#include <debug.h>
#include <string.h>
#include "ext2.h"
#include "request.h"
#include "inodecache.h"
#include "file.h"

static u32 ext2_getBlockOfInode(sExt2 *e,sInode *inode,u32 block);

s32 ext2_readFile(sExt2 *e,tInodeNo inodeNo,u8 *buffer,u32 offset,u32 count) {
	sCachedInode *cnode;
	u8 *tmpBuffer;
	u8 *bufWork;
	u32 i,blockSize,startBlock,blockCount,leftBytes;

	/* at first we need the inode */
	cnode = ext2_icache_request(e,inodeNo);
	if(cnode == NULL)
		return 0;

	/* nothing left to read? */
	if((s32)offset < 0 || (s32)offset >= cnode->inode.size)
		return 0;
	/* adjust count */
	if((s32)(offset + count) < 0 || (s32)(offset + count) >= cnode->inode.size)
		count = cnode->inode.size - offset;

	blockSize = BLOCK_SIZE(e);
	startBlock = offset / blockSize;
	blockCount = (count + blockSize - 1) / blockSize;

	tmpBuffer = (u8*)malloc(blockSize * sizeof(u8));
	if(tmpBuffer == NULL)
		return 0;

	/* TODO try to read multiple blocks at once */

	/* use the offset in the first block; after the first one the offset is 0 anyway */
	offset %= blockSize;
	leftBytes = count;
	bufWork = buffer;
	for(i = 0; i < blockCount; i++) {
		u32 block = ext2_getBlockOfInode(e,&(cnode->inode),startBlock + i);

		ext2_readBlocks(e,tmpBuffer,block,1);
		memcpy(bufWork,tmpBuffer + offset,MIN(leftBytes,blockSize - offset));

		bufWork += blockSize;
		/* offset is always 0 for additional blocks */
		offset = 0;
		/* we substract to much, but it matters only if we read an additional block. In this
		 * case it is correct */
		leftBytes -= blockSize - offset;
	}

	free(tmpBuffer);
	return count;
}

static u32 ext2_getBlockOfInode(sExt2 *e,sInode *inode,u32 block) {
	u32 i,blockSize,blocksPerBlock;
	u32 blperBlSq;
	u32 *buffer;

	/* direct block */
	if(block < EXT2_DIRBLOCK_COUNT)
		return inode->dBlocks[block];

	/* singly indirect */
	block -= EXT2_DIRBLOCK_COUNT;
	blockSize = BLOCK_SIZE(e);
	blocksPerBlock = blockSize / sizeof(u32);
	if(block < blocksPerBlock) {
		buffer = (u32*)malloc(blockSize);
		ext2_readBlocks(e,(u8*)buffer,inode->singlyIBlock,1);
		i = *(buffer + block);
		free(buffer);
		return i;
	}

	/* TODO we have to verify if this is all correct here... */

	/* doubly indirect */
	block -= blocksPerBlock;
	blperBlSq = blocksPerBlock * blocksPerBlock;
	if(block < blperBlSq) {
		buffer = (u32*)malloc(blockSize);

		/* read the first block with block-numbers of the indirect blocks */
		ext2_readBlocks(e,(u8*)buffer,inode->doublyIBlock,1);
		i = *(buffer + block / blocksPerBlock);

		/* read the indirect block */
		ext2_readBlocks(e,(u8*)buffer,i,1);
		i = *(buffer + block % blocksPerBlock);

		free(buffer);
		return i;
	}

	/* triply indirect */
	block -= blperBlSq;
	buffer = (u32*)malloc(blockSize);

	/* read the first block with block-numbers of the indirect blocks of indirect-blocks */
	ext2_readBlocks(e,(u8*)buffer,inode->triplyIBlock,1);
	i = *(buffer + block / blperBlSq);

	/* read the indirect block of indirect blocks */
	block %= blperBlSq;
	ext2_readBlocks(e,(u8*)buffer,i,1);
	i = *(buffer + block / blocksPerBlock);

	/* read the indirect block */
	ext2_readBlocks(e,(u8*)buffer,i,1);
	i = *(buffer + block % blocksPerBlock);

	free(buffer);
	return i;
}
