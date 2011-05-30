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
#include <stdio.h>
#include <string.h>
#include <errors.h>
#include <time.h>

#include "ext2.h"
#include "inode.h"
#include "superblock.h"
#include "inodecache.h"
#include "../blockcache.h"
#include "bitmap.h"

/**
 * Performs the actual get-block-request. If <req> is true, it will allocate a new block, if
 * necessary. In this case cnode may be changed. Otherwise no changes will be made.
 */
static tBlockNo ext2_inode_doGetDataBlock(sExt2 *e,sExt2CInode *cnode,tBlockNo block,bool req);
/**
 * Puts a new block in cblock->buffer if cblock->buffer[index] is 0. Marks the cblock dirty,
 * if necessary. Sets <added> to true or false, depending on whether a block was allocated.
 */
static int ext2_inode_extend(sExt2 *e,sExt2CInode *cnode,sCBlock *cblock,size_t index,bool *added);

int ext2_inode_create(sExt2 *e,sExt2CInode *dirNode,sExt2CInode **ino,bool isDir) {
	size_t i;
	tTime now;
	sExt2CInode *cnode;

	/* request inode */
	tInodeNo inodeNo = ext2_bm_allocInode(e,dirNode,isDir);
	if(inodeNo == 0)
		return ERR_INO_ALLOC;
	cnode = ext2_icache_request(e,inodeNo,IMODE_WRITE);
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
	now = time(NULL);
	cnode->inode.accesstime = now;
	cnode->inode.createtime = now;
	cnode->inode.modifytime = now;
	cnode->inode.deletetime = 0;
	ext2_icache_markDirty(cnode);
	*ino = cnode;
	return 0;
}

int ext2_inode_destroy(sExt2 *e,sExt2CInode *cnode) {
	int res;
	/* free inode, clear it and ensure that it get's written back to disk */
	if((res = ext2_bm_freeInode(e,cnode->inodeNo,MODE_IS_DIR(cnode->inode.mode))) < 0)
		return res;
	/* just set the delete-time and reset link-count. the block-numbers in the inode
	 * are still present, so that it may be possible to restore the file, if the blocks
	 * have not been overwritten in the meantime. */
	cnode->inode.deletetime = time(NULL);
	cnode->inode.linkCount = 0;
	ext2_icache_markDirty(cnode);
	return 0;
}

tBlockNo ext2_inode_reqDataBlock(sExt2 *e,sExt2CInode *cnode,tBlockNo block) {
	return ext2_inode_doGetDataBlock(e,cnode,block,true);
}

tBlockNo ext2_inode_getDataBlock(sExt2 *e,const sExt2CInode *cnode,tBlockNo block) {
	return ext2_inode_doGetDataBlock(e,(sExt2CInode*)cnode,block,false);
}

static tBlockNo ext2_inode_doGetDataBlock(sExt2 *e,sExt2CInode *cnode,tBlockNo block,bool req) {
	tBlockNo i;
	size_t blockSize,blocksPerBlock,blperBlSq;
	uint bmode = req ? BMODE_WRITE : BMODE_READ;
	bool added = false;
	sCBlock *cblock;

	/* Note that we don't have to mark the inode dirty here if blocks are added
	 * because ext2_file_write() does it for us */

	/* direct block */
	if(block < EXT2_DIRBLOCK_COUNT) {
		/* alloc a new block if necessary */
		if(req && cnode->inode.dBlocks[block] == 0) {
			cnode->inode.dBlocks[block] = ext2_bm_allocBlock(e,cnode);
			if(cnode->inode.dBlocks[block] != 0)
				cnode->inode.blocks += EXT2_BLKS_TO_SECS(e,1);
		}
		return cnode->inode.dBlocks[block];
	}

	/* singly indirect */
	block -= EXT2_DIRBLOCK_COUNT;
	blockSize = EXT2_BLK_SIZE(e);
	blocksPerBlock = blockSize / sizeof(tBlockNo);
	if(block < blocksPerBlock) {
		added = false;
		/* no singly-indirect-block present yet? */
		if(cnode->inode.singlyIBlock == 0) {
			if(!req)
				return 0;
			cnode->inode.singlyIBlock = ext2_bm_allocBlock(e,cnode);
			if(cnode->inode.singlyIBlock == 0)
				return 0;
			cnode->inode.blocks += EXT2_BLKS_TO_SECS(e,1);
			added = true;
		}

		cblock = bcache_request(&e->blockCache,cnode->inode.singlyIBlock,bmode);
		if(cblock == NULL)
			return 0;
		if(added) {
			memclear(cblock->buffer,EXT2_BLK_SIZE(e));
			bcache_markDirty(cblock);
		}
		if(req && ext2_inode_extend(e,cnode,cblock,block,&added) != 1) {
			bcache_release(cblock);
			return 0;
		}
		i = *((tBlockNo*)(cblock->buffer) + block);
		bcache_release(cblock);
		return i;
	}

	/* TODO we have to verify if this is all correct here... */

	/* doubly indirect */
	block -= blocksPerBlock;
	blperBlSq = blocksPerBlock * blocksPerBlock;
	if(block < blperBlSq) {
		added = false;
		/* no doubly-indirect-block present yet? */
		if(cnode->inode.doublyIBlock == 0) {
			if(!req)
				return 0;
			cnode->inode.doublyIBlock = ext2_bm_allocBlock(e,cnode);
			if(cnode->inode.doublyIBlock == 0)
				return 0;
			added = true;
			cnode->inode.blocks += EXT2_BLKS_TO_SECS(e,1);
		}

		/* read the first block with block-numbers of the indirect blocks */
		cblock = bcache_request(&e->blockCache,cnode->inode.doublyIBlock,bmode);
		if(cblock == NULL)
			return 0;
		if(added) {
			memclear(cblock->buffer,EXT2_BLK_SIZE(e));
			bcache_markDirty(cblock);
		}
		if(req && ext2_inode_extend(e,cnode,cblock,block / blocksPerBlock,&added) != 1) {
			bcache_release(cblock);
			return 0;
		}
		i = *((tBlockNo*)(cblock->buffer) + block / blocksPerBlock);
		bcache_release(cblock);

		/* may happen if we should not request new blocks */
		if(i == 0)
			return 0;

		/* read the indirect block */
		cblock = bcache_request(&e->blockCache,i,bmode);
		if(cblock == NULL)
			return 0;
		if(added) {
			memclear(cblock->buffer,EXT2_BLK_SIZE(e));
			bcache_markDirty(cblock);
		}
		if(req && ext2_inode_extend(e,cnode,cblock,block % blocksPerBlock,&added) != 1) {
			bcache_release(cblock);
			return 0;
		}
		i = *((tBlockNo*)(cblock->buffer) + block % blocksPerBlock);
		bcache_release(cblock);

		return i;
	}

	/* triply indirect */
	block -= blperBlSq;

	added = false;
	/* no triply-indirect-block present yet? */
	if(cnode->inode.triplyIBlock == 0) {
		if(!req)
			return 0;
		cnode->inode.triplyIBlock = ext2_bm_allocBlock(e,cnode);
		if(cnode->inode.triplyIBlock == 0)
			return 0;
		added = true;
		cnode->inode.blocks += EXT2_BLKS_TO_SECS(e,1);
	}

	/* read the first block with block-numbers of the indirect blocks of indirect-blocks */
	cblock = bcache_request(&e->blockCache,cnode->inode.triplyIBlock,bmode);
	if(cblock == NULL)
		return 0;
	if(added) {
		memclear(cblock->buffer,EXT2_BLK_SIZE(e));
		bcache_markDirty(cblock);
	}
	if(req && ext2_inode_extend(e,cnode,cblock,block / blperBlSq,&added) != 1) {
		bcache_release(cblock);
		return 0;
	}
	i = *((tBlockNo*)(cblock->buffer) + block / blperBlSq);
	bcache_release(cblock);

	if(i == 0)
		return 0;

	/* read the indirect block of indirect blocks */
	block %= blperBlSq;
	cblock = bcache_request(&e->blockCache,i,bmode);
	if(cblock == NULL)
		return 0;
	if(added) {
		memclear(cblock->buffer,EXT2_BLK_SIZE(e));
		bcache_markDirty(cblock);
	}
	if(req && ext2_inode_extend(e,cnode,cblock,block / blocksPerBlock,&added) != 1) {
		bcache_release(cblock);
		return 0;
	}
	i = *((tBlockNo*)(cblock->buffer) + block / blocksPerBlock);
	bcache_release(cblock);

	if(i == 0)
		return 0;

	/* read the indirect block */
	cblock = bcache_request(&e->blockCache,i,bmode);
	if(cblock == NULL)
		return 0;
	if(added) {
		memclear(cblock->buffer,EXT2_BLK_SIZE(e));
		bcache_markDirty(cblock);
	}
	if(req && ext2_inode_extend(e,cnode,cblock,block % blocksPerBlock,&added) != 1) {
		bcache_release(cblock);
		return 0;
	}
	i = *((tBlockNo*)(cblock->buffer) + block % blocksPerBlock);
	bcache_release(cblock);

	return i;
}

static int ext2_inode_extend(sExt2 *e,sExt2CInode *cnode,sCBlock *cblock,size_t index,bool *added) {
	tBlockNo *blockNos = (tBlockNo*)(cblock->buffer);
	if(blockNos[index] == 0) {
		tBlockNo bno = ext2_bm_allocBlock(e,cnode);
		if(bno == 0)
			return 0;
		blockNos[index] = bno;
		cnode->inode.blocks += EXT2_BLKS_TO_SECS(e,1);
		bcache_markDirty(cblock);
		*added = true;
	}
	else
		*added = false;
	return 1;
}

#if DEBUGGING

void ext2_inode_print(sExt2Inode *inode) {
	size_t i;
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
