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
#include <esc/date.h>
#include <string.h>
#include <errors.h>
#include "ext2.h"
#include "inode.h"
#include "superblock.h"
#include "inodecache.h"
#include "blockcache.h"
#include "bitmap.h"

/**
 * Puts a new block in cblock->buffer if cblock->buffer[index] is 0. Marks the cblock dirty,
 * if necessary. Sets <added> to true or false, depending on wether a block was allocated.
 */
static u32 ext2_inode_extend(sExt2 *e,sExt2CInode *cnode,sExt2CBlock *cblock,u32 index,bool *added);

s32 ext2_inode_create(sExt2 *e,sExt2CInode *dirNode,sExt2CInode **ino,bool isDir) {
	u32 i,now;
	sExt2CInode *cnode;

	/* request inode */
	tInodeNo inodeNo = ext2_bm_allocInode(e,dirNode,isDir);
	if(inodeNo == 0)
		return ERR_INO_ALLOC;
	cnode = ext2_icache_request(e,inodeNo);
	if(cnode == NULL) {
		ext2_bm_freeInode(e,inodeNo,isDir);
		return ERR_INO_REQ_FAILED;
	}

	/* init inode */
	cnode->inode.gid = 0;
	cnode->inode.uid = 0;
	if(isDir) {
		cnode->inode.mode = EXT2_S_IFDIR |
			EXT2_S_IRUSR | EXT2_S_IRGRP | EXT2_S_IROTH |
			EXT2_S_IXUSR | EXT2_S_IXGRP | EXT2_S_IXOTH |
			EXT2_S_IWUSR;
	}
	else {
		cnode->inode.mode = EXT2_S_IFREG |
			EXT2_S_IRUSR | EXT2_S_IWUSR | EXT2_S_IRGRP | EXT2_S_IROTH;
	}
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
	cnode->inode.deletetime = 0;
	cnode->dirty = true;
	*ino = cnode;
	return 0;
}

s32 ext2_inode_destroy(sExt2 *e,sExt2CInode *cnode) {
	s32 res;
	/* free inode, clear it and ensure that it get's written back to disk */
	if((res = ext2_bm_freeInode(e,cnode->inodeNo,MODE_IS_DIR(cnode->inode.mode))) < 0)
		return res;
	/* just set the delete-time and reset link-count. the block-numbers in the inode
	 * are still present, so that it may be possible to restore the file, if the blocks
	 * have not been overwritten in the meantime. */
	cnode->inode.deletetime = getTime();
	cnode->inode.linkCount = 0;
	cnode->dirty = true;
	return 0;
}

u32 ext2_inode_getDataBlock(sExt2 *e,sExt2CInode *cnode,u32 block) {
	u32 i,blockSize,blocksPerBlock,blperBlSq;
	bool added = false;
	sExt2CBlock *cblock;

	/* Note that we don't have to mark the inode dirty here if blocks are added
	 * because ext2_file_write() does it for us */

	/* direct block */
	if(block < EXT2_DIRBLOCK_COUNT) {
		/* alloc a new block if necessary */
		if(cnode->inode.dBlocks[block] == 0) {
			cnode->inode.dBlocks[block] = ext2_bm_allocBlock(e,cnode);
			if(cnode->inode.dBlocks[block] != 0)
				cnode->inode.blocks += EXT2_BLKS_TO_SECS(e,1);
		}
		return cnode->inode.dBlocks[block];
	}

	/* singly indirect */
	block -= EXT2_DIRBLOCK_COUNT;
	blockSize = EXT2_BLK_SIZE(e);
	blocksPerBlock = blockSize / sizeof(u32);
	if(block < blocksPerBlock) {
		/* no singly-indirect-block present yet? */
		if(cnode->inode.singlyIBlock == 0) {
			cnode->inode.singlyIBlock = ext2_bm_allocBlock(e,cnode);
			if(cnode->inode.singlyIBlock == 0)
				return 0;
			cnode->inode.blocks += EXT2_BLKS_TO_SECS(e,1);
			added = true;
		}

		cblock = ext2_bcache_request(e,cnode->inode.singlyIBlock);
		if(cblock == NULL)
			return 0;
		if(added) {
			memclear(cblock->buffer,EXT2_BLK_SIZE(e));
			cblock->dirty = true;
		}
		if(ext2_inode_extend(e,cnode,cblock,block,&added) != 1)
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
			cnode->inode.doublyIBlock = ext2_bm_allocBlock(e,cnode);
			if(cnode->inode.doublyIBlock == 0)
				return 0;
			added = true;
			cnode->inode.blocks += EXT2_BLKS_TO_SECS(e,1);
		}

		/* read the first block with block-numbers of the indirect blocks */
		cblock = ext2_bcache_request(e,cnode->inode.doublyIBlock);
		if(cblock == NULL)
			return 0;
		if(added) {
			memclear(cblock->buffer,EXT2_BLK_SIZE(e));
			cblock->dirty = true;
		}
		if(ext2_inode_extend(e,cnode,cblock,block / blocksPerBlock,&added) != 1)
			return 0;
		i = *((u32*)(cblock->buffer) + block / blocksPerBlock);

		/* read the indirect block */
		cblock = ext2_bcache_request(e,i);
		if(cblock == NULL)
			return 0;
		if(added) {
			memclear(cblock->buffer,EXT2_BLK_SIZE(e));
			cblock->dirty = true;
		}
		if(ext2_inode_extend(e,cnode,cblock,block % blocksPerBlock,&added) != 1)
			return 0;
		i = *((u32*)(cblock->buffer) + block % blocksPerBlock);

		return i;
	}

	/* triply indirect */
	block -= blperBlSq;

	/* no triply-indirect-block present yet? */
	if(cnode->inode.triplyIBlock == 0) {
		cnode->inode.triplyIBlock = ext2_bm_allocBlock(e,cnode);
		if(cnode->inode.triplyIBlock == 0)
			return 0;
		added = true;
		cnode->inode.blocks += EXT2_BLKS_TO_SECS(e,1);
	}

	/* read the first block with block-numbers of the indirect blocks of indirect-blocks */
	cblock = ext2_bcache_request(e,cnode->inode.triplyIBlock);
	if(cblock == NULL)
		return 0;
	if(added) {
		memclear(cblock->buffer,EXT2_BLK_SIZE(e));
		cblock->dirty = true;
	}
	if(ext2_inode_extend(e,cnode,cblock,block / blperBlSq,&added) != 1)
		return 0;
	i = *((u32*)(cblock->buffer) + block / blperBlSq);

	/* read the indirect block of indirect blocks */
	block %= blperBlSq;
	cblock = ext2_bcache_request(e,i);
	if(cblock == NULL)
		return 0;
	if(added) {
		memclear(cblock->buffer,EXT2_BLK_SIZE(e));
		cblock->dirty = true;
	}
	if(ext2_inode_extend(e,cnode,cblock,block / blocksPerBlock,&added) != 1)
		return 0;
	i = *((u32*)(cblock->buffer) + block / blocksPerBlock);

	/* read the indirect block */
	cblock = ext2_bcache_request(e,i);
	if(cblock == NULL)
		return 0;
	if(added) {
		memclear(cblock->buffer,EXT2_BLK_SIZE(e));
		cblock->dirty = true;
	}
	if(ext2_inode_extend(e,cnode,cblock,block % blocksPerBlock,&added) != 1)
		return 0;
	i = *((u32*)(cblock->buffer) + block % blocksPerBlock);

	return i;
}

static u32 ext2_inode_extend(sExt2 *e,sExt2CInode *cnode,sExt2CBlock *cblock,u32 index,bool *added) {
	u32 *blockNos = (u32*)(cblock->buffer);
	if(blockNos[index] == 0) {
		u32 bno = ext2_bm_allocBlock(e,cnode);
		if(bno == 0)
			return 0;
		blockNos[index] = bno;
		cnode->inode.blocks += EXT2_BLKS_TO_SECS(e,1);
		cblock->dirty = true;
		*added = true;
	}
	else
		*added = false;
	return 1;
}

#if DEBUGGING

void ext2_inode_print(sExt2Inode *inode) {
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
