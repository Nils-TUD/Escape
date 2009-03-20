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
#include "inode.h"
#include "blockcache.h"
#include "inodecache.h"
#include "file.h"

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

	/* TODO try to read multiple blocks at once */

	/* use the offset in the first block; after the first one the offset is 0 anyway */
	offset %= blockSize;
	leftBytes = count;
	bufWork = buffer;
	for(i = 0; i < blockCount; i++) {
		u32 block = ext2_getBlockOfInode(e,&(cnode->inode),startBlock + i);

		/* request block */
		tmpBuffer = ext2_bcache_request(e,block);
		if(tmpBuffer == NULL)
			return 0;

		/* copy the requested part */
		memcpy(bufWork,tmpBuffer + offset,MIN(leftBytes,blockSize - offset));

		bufWork += blockSize;
		/* offset is always 0 for additional blocks */
		offset = 0;
		/* we substract to much, but it matters only if we read an additional block. In this
		 * case it is correct */
		leftBytes -= blockSize - offset;
	}

	return count;
}
