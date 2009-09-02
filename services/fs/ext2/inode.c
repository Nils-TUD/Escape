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
#include <esc/io.h>
#include <esc/debug.h>
#include <string.h>
#include "ext2.h"
#include "inode.h"
#include "superblock.h"
#include "blockcache.h"

/**
 * Puts a new block in cblock->buffer if cblock->buffer[index] is 0. Marks the cblock dirty,
 * if necessary. Sets <added> to true or false, depending on wether a block was allocated.
 */
static u32 ext2_extend(sExt2 *e,sCachedInode *cnode,sBCacheEntry *cblock,u32 index,bool *added);

u32 ext2_getDataBlock(sExt2 *e,sCachedInode *cnode,u32 block) {
	u32 i,blockSize,blocksPerBlock,blperBlSq,bno;
	bool added = false;
	sBCacheEntry *cblock;

	/* Note that we don't have to mark the inode dirty here if blocks are added
	 * because ext2_writeFile() does it for us */

	/* direct block */
	if(block < EXT2_DIRBLOCK_COUNT) {
		/* alloc a new block if necessary */
		if(cnode->inode.dBlocks[block] == 0)
			cnode->inode.dBlocks[block] = ext2_allocBlock(e,cnode);
		return cnode->inode.dBlocks[block];
	}

	/* singly indirect */
	block -= EXT2_DIRBLOCK_COUNT;
	blockSize = BLOCK_SIZE(e);
	blocksPerBlock = blockSize / sizeof(u32);
	if(block < blocksPerBlock) {
		/* no singly-indirect-block present yet? */
		if(cnode->inode.singlyIBlock == 0) {
			cnode->inode.singlyIBlock = ext2_allocBlock(e,cnode);
			if(cnode->inode.singlyIBlock == 0)
				return 0;
			added = true;
		}

		cblock = ext2_bcache_request(e,cnode->inode.singlyIBlock);
		if(cblock == NULL)
			return 0;
		if(added) {
			memclear(cblock->buffer,BLOCK_SIZE(e));
			cblock->dirty = true;
		}
		if(ext2_extend(e,cnode,cblock,block,&added) != 1)
			return 0;
		i = *((u32*)(cblock->buffer) + block);
		return i;
	}

	/* TODO we have to verify if this is all correct here... */

	/* doubly indirect */
	block -= blocksPerBlock;
	blperBlSq = blocksPerBlock * blocksPerBlock;
	if(block < blperBlSq) {
		/* no doubly-indirect-block present yet? */
		if(cnode->inode.doublyIBlock == 0) {
			cnode->inode.doublyIBlock = ext2_allocBlock(e,cnode);
			if(cnode->inode.doublyIBlock == 0)
				return 0;
			added = true;
		}

		/* read the first block with block-numbers of the indirect blocks */
		cblock = ext2_bcache_request(e,cnode->inode.doublyIBlock);
		if(cblock == NULL)
			return 0;
		if(added) {
			memclear(cblock->buffer,BLOCK_SIZE(e));
			cblock->dirty = true;
		}
		if(ext2_extend(e,cnode,cblock,block / blocksPerBlock,&added) != 1)
			return 0;
		i = *((u32*)(cblock->buffer) + block / blocksPerBlock);

		/* read the indirect block */
		cblock = ext2_bcache_request(e,i);
		if(cblock == NULL)
			return 0;
		if(added) {
			memclear(cblock->buffer,BLOCK_SIZE(e));
			cblock->dirty = true;
		}
		if(ext2_extend(e,cnode,cblock,block % blocksPerBlock,&added) != 1)
			return 0;
		i = *((u32*)(cblock->buffer) + block % blocksPerBlock);

		return i;
	}

	/* triply indirect */
	block -= blperBlSq;

	/* no triply-indirect-block present yet? */
	if(cnode->inode.triplyIBlock == 0) {
		cnode->inode.triplyIBlock = ext2_allocBlock(e,cnode);
		if(cnode->inode.triplyIBlock == 0)
			return 0;
		added = true;
	}

	/* read the first block with block-numbers of the indirect blocks of indirect-blocks */
	cblock = ext2_bcache_request(e,cnode->inode.triplyIBlock);
	if(cblock == NULL)
		return 0;
	if(added) {
		memclear(cblock->buffer,BLOCK_SIZE(e));
		cblock->dirty = true;
	}
	if(ext2_extend(e,cnode,cblock,block / blperBlSq,&added) != 1)
		return 0;
	i = *((u32*)(cblock->buffer) + block / blperBlSq);

	/* read the indirect block of indirect blocks */
	block %= blperBlSq;
	cblock = ext2_bcache_request(e,i);
	if(cblock == NULL)
		return 0;
	if(added) {
		memclear(cblock->buffer,BLOCK_SIZE(e));
		cblock->dirty = true;
	}
	if(ext2_extend(e,cnode,cblock,block / blocksPerBlock,&added) != 1)
		return 0;
	i = *((u32*)(cblock->buffer) + block / blocksPerBlock);

	/* read the indirect block */
	cblock = ext2_bcache_request(e,i);
	if(cblock == NULL)
		return 0;
	if(added) {
		memclear(cblock->buffer,BLOCK_SIZE(e));
		cblock->dirty = true;
	}
	if(ext2_extend(e,cnode,cblock,block % blocksPerBlock,&added) != 1)
		return 0;
	i = *((u32*)(cblock->buffer) + block % blocksPerBlock);

	return i;
}

static u32 ext2_extend(sExt2 *e,sCachedInode *cnode,sBCacheEntry *cblock,u32 index,bool *added) {
	u32 *blockNos = (u32*)(cblock->buffer);
	if(blockNos[index] == 0) {
		u32 bno = ext2_allocBlock(e,cnode);
		if(bno == 0)
			return 0;
		blockNos[index] = bno;
		cblock->dirty = true;
		*added = true;
	}
	else
		*added = false;
	return 1;
}

#if DEBUGGING

void ext2_dbg_printInode(sInode *inode) {
	u32 i;
	debugf("\tmode=0x%08x\n",inode->mode);
	debugf("\tuid=%d\n",inode->uid);
	debugf("\tgid=%d\n",inode->gid);
	debugf("\tsize=%d\n",inode->size);
	debugf("\taccesstime=%d\n",inode->accesstime);
	debugf("\tcreatetime=%d\n",inode->createtime);
	debugf("\tmodifytime=%d\n",inode->modifytime);
	debugf("\tdeletetime=%d\n",inode->deletetime);
	debugf("\tlinkCount=%d\n",inode->linkCount);
	debugf("\tblocks=%d\n",inode->blocks);
	debugf("\tflags=0x%08x\n",inode->flags);
	debugf("\tosd1=0x%08x\n",inode->osd1);
	for(i = 0; i < EXT2_DIRBLOCK_COUNT; i++)
		debugf("\tblock%d=%d\n",i,inode->dBlocks[i]);
	debugf("\tsinglyIBlock=%d\n",inode->singlyIBlock);
	debugf("\tdoublyIBlock=%d\n",inode->doublyIBlock);
	debugf("\ttriplyIBlock=%d\n",inode->triplyIBlock);
	debugf("\tgeneration=%d\n",inode->generation);
	debugf("\tfileACL=%d\n",inode->fileACL);
	debugf("\tdirACL=%d\n",inode->dirACL);
	debugf("\tfragAddr=%d\n",inode->fragAddr);
	debugf("\tosd2=0x%08x%08x%08x%08x\n",inode->osd2[0],inode->osd2[1],inode->osd2[2],inode->osd2[3]);
}

#endif
