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
#include "ext2.h"
#include "inode.h"
#include "blockcache.h"

u32 ext2_getBlockOfInode(sExt2 *e,sInode *inode,u32 block) {
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
		buffer = (u32*)ext2_bcache_request(e,inode->singlyIBlock);
		if(buffer == NULL)
			return 0;
		i = *(buffer + block);
		return i;
	}

	/* TODO we have to verify if this is all correct here... */

	/* doubly indirect */
	block -= blocksPerBlock;
	blperBlSq = blocksPerBlock * blocksPerBlock;
	if(block < blperBlSq) {
		/* read the first block with block-numbers of the indirect blocks */
		buffer = (u32*)ext2_bcache_request(e,inode->doublyIBlock);
		if(buffer == NULL)
			return 0;
		i = *(buffer + block / blocksPerBlock);

		/* read the indirect block */
		buffer = (u32*)ext2_bcache_request(e,i);
		if(buffer == NULL)
			return 0;
		i = *(buffer + block % blocksPerBlock);

		return i;
	}

	/* triply indirect */
	block -= blperBlSq;

	/* read the first block with block-numbers of the indirect blocks of indirect-blocks */
	buffer = (u32*)ext2_bcache_request(e,inode->triplyIBlock);
	if(buffer == NULL)
		return 0;
	i = *(buffer + block / blperBlSq);

	/* read the indirect block of indirect blocks */
	block %= blperBlSq;
	buffer = (u32*)ext2_bcache_request(e,i);
	if(buffer == NULL)
		return 0;
	i = *(buffer + block / blocksPerBlock);

	/* read the indirect block */
	buffer = (u32*)ext2_bcache_request(e,i);
	if(buffer == NULL)
		return 0;
	i = *(buffer + block % blocksPerBlock);

	return i;
}

#if DEBUGGING

void ext2_dbg_printInode(sInode *inode) {
	u32 i;
	printf("\tmode=0x%08x\n",inode->mode);
	printf("\tuid=%d\n",inode->uid);
	printf("\tgid=%d\n",inode->gid);
	printf("\tsize=%d\n",inode->size);
	printf("\taccesstime=%d\n",inode->accesstime);
	printf("\tcreatetime=%d\n",inode->createtime);
	printf("\tmodifytime=%d\n",inode->modifytime);
	printf("\tdeletetime=%d\n",inode->deletetime);
	printf("\tlinkCount=%d\n",inode->linkCount);
	printf("\tblocks=%d\n",inode->blocks);
	printf("\tflags=0x%08x\n",inode->flags);
	printf("\tosd1=0x%08x\n",inode->osd1);
	for(i = 0; i < EXT2_DIRBLOCK_COUNT; i++)
		printf("\tblock%d=%d\n",i,inode->dBlocks[i]);
	printf("\tsinglyIBlock=%d\n",inode->singlyIBlock);
	printf("\tdoublyIBlock=%d\n",inode->doublyIBlock);
	printf("\ttriplyIBlock=%d\n",inode->triplyIBlock);
	printf("\tgeneration=%d\n",inode->generation);
	printf("\tfileACL=%d\n",inode->fileACL);
	printf("\tdirACL=%d\n",inode->dirACL);
	printf("\tfragAddr=%d\n",inode->fragAddr);
	printf("\tosd2=0x%08x%08x%08x%08x\n",inode->osd2[0],inode->osd2[1],inode->osd2[2],inode->osd2[3]);
}

#endif
