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

#include <esc/common.h>
#include <esc/io.h>
#include <esc/endian.h>
#include <esc/proc.h>
#include <fs/blockcache.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "ext2.h"
#include "inode.h"
#include "sbmng.h"
#include "inodecache.h"
#include "bitmap.h"

int Ext2INode::create(Ext2FileSystem *e,FSUser *u,Ext2CInode *dirNode,Ext2CInode **ino,bool isDir) {
	size_t i;
	time_t now;
	Ext2CInode *cnode;

	/* request inode */
	inode_t inodeNo = Ext2Bitmap::allocInode(e,dirNode,isDir);
	if(inodeNo == 0)
		return -ENOSPC;
	cnode = e->inodeCache.request(inodeNo,IMODE_WRITE);
	if(cnode == NULL) {
		Ext2Bitmap::freeInode(e,inodeNo,isDir);
		return -ENOBUFS;
	}

	/* init inode */
	cnode->inode.uid = cputole16(u->uid);
	cnode->inode.gid = cputole16(u->gid);
	if(isDir) {
		/* drwxr-xr-x */
		cnode->inode.mode = cputole16(EXT2_S_IFDIR |
			EXT2_S_IRUSR | EXT2_S_IRGRP | EXT2_S_IROTH |
			EXT2_S_IXUSR | EXT2_S_IXGRP | EXT2_S_IXOTH |
			EXT2_S_IWUSR);
	}
	else {
		/* -rw-r--r-- */
		cnode->inode.mode = cputole16(EXT2_S_IFREG |
			EXT2_S_IRUSR | EXT2_S_IWUSR | EXT2_S_IRGRP | EXT2_S_IROTH);
	}
	cnode->inode.linkCount = cputole16(0);
	cnode->inode.size = cputole32(0);
	cnode->inode.singlyIBlock = cputole32(0);
	cnode->inode.doublyIBlock = cputole32(0);
	cnode->inode.triplyIBlock = cputole32(0);
	for(i = 0; i < EXT2_DIRBLOCK_COUNT; i++)
		cnode->inode.dBlocks[i] = cputole32(0);
	cnode->inode.blocks = cputole32(0);
	now = cputole32(e->timestamp());
	cnode->inode.accesstime = now;
	cnode->inode.createtime = now;
	cnode->inode.modifytime = now;
	cnode->inode.deletetime = cputole32(0);
	e->inodeCache.markDirty(cnode);
	*ino = cnode;
	return 0;
}

int Ext2INode::chmod(Ext2FileSystem *e,FSUser *u,inode_t inodeNo,mode_t mode) {
	mode_t oldMode;
	Ext2CInode *cnode = e->inodeCache.request(inodeNo,IMODE_WRITE);
	if(cnode == NULL)
		return -ENOBUFS;

	/* root can chmod everything; others can only chmod their own files */
	if(u->uid != le16tocpu(cnode->inode.uid) && u->uid != ROOT_UID)
		return -EPERM;

	oldMode = le16tocpu(cnode->inode.mode);
	cnode->inode.mode = cputole16((oldMode & ~EXT2_S_PERMS) | (mode & EXT2_S_PERMS));
	e->inodeCache.markDirty(cnode);
	e->inodeCache.release(cnode);
	return 0;
}

int Ext2INode::chown(Ext2FileSystem *e,FSUser *u,inode_t inodeNo,uid_t uid,gid_t gid) {
	uid_t oldUid;
	gid_t oldGid;
	Ext2CInode *cnode = e->inodeCache.request(inodeNo,IMODE_WRITE);
	if(cnode == NULL)
		return -ENOBUFS;

	/* root can chown everything; others can only chown their own files */
	oldUid = le16tocpu(cnode->inode.uid);
	oldGid = le16tocpu(cnode->inode.gid);
	if(u->uid != oldUid && u->uid != ROOT_UID)
		return -EPERM;
	if(u->uid != ROOT_UID) {
		/* users can't change the owner */
		if(uid != (uid_t)-1 && uid != oldUid && uid != u->uid)
			return -EPERM;
		/* users can change the group only to a group they're a member of */
		if(gid != (gid_t)-1 && gid != oldGid && gid != u->gid && !isingroup(u->pid,gid))
			return -EPERM;
	}

	if(uid != (uid_t)-1)
		cnode->inode.uid = cputole16(uid);
	if(gid != (gid_t)-1)
		cnode->inode.gid = cputole16(gid);
	e->inodeCache.markDirty(cnode);
	e->inodeCache.release(cnode);
	return 0;
}

int Ext2INode::destroy(Ext2FileSystem *e,Ext2CInode *cnode) {
	int res;
	/* free inode, clear it and ensure that it get's written back to disk */
	if((res = Ext2Bitmap::freeInode(e,cnode->inodeNo,S_ISDIR(le16tocpu(cnode->inode.mode)))) < 0)
		return res;
	/* just set the delete-time and reset link-count. the block-numbers in the inode
	 * are still present, so that it may be possible to restore the file, if the blocks
	 * have not been overwritten in the meantime. */
	cnode->inode.deletetime = cputole32(e->timestamp());
	cnode->inode.linkCount = cputole16(0);
	e->inodeCache.markDirty(cnode);
	return 0;
}

block_t Ext2INode::doGetDataBlock(Ext2FileSystem *e,Ext2CInode *cnode,block_t block,bool req) {
	block_t i;
	size_t blockSize,blocksPerBlock,blperBlSq;
	uint bmode = req ? BlockCache::WRITE : BlockCache::READ;
	bool added = false;
	CBlock *cblock;

	/* Note that we don't have to mark the inode dirty here if blocks are added
	 * because ext2_file_write() does it for us */

	/* direct block */
	if(block < EXT2_DIRBLOCK_COUNT) {
		/* alloc a new block if necessary */
		i = le32tocpu(cnode->inode.dBlocks[block]);
		if(req && i == 0) {
			i = Ext2Bitmap::allocBlock(e,cnode);
			cnode->inode.dBlocks[block] = cputole32(i);
			if(i != 0) {
				uint32_t blocks = le32tocpu(cnode->inode.blocks);
				cnode->inode.blocks = cputole32(blocks + e->blocksToSecs(1));
			}
		}
		return i;
	}

	/* singly indirect */
	block -= EXT2_DIRBLOCK_COUNT;
	blockSize = e->blockSize();
	blocksPerBlock = blockSize / sizeof(block_t);
	if(block < blocksPerBlock) {
		added = false;
		i = le32tocpu(cnode->inode.singlyIBlock);
		/* no singly-indirect-block present yet? */
		if(i == 0) {
			if(!req)
				return 0;
			i = Ext2Bitmap::allocBlock(e,cnode);
			cnode->inode.singlyIBlock = cputole32(i);
			if(i == 0)
				return 0;
			cnode->inode.blocks = cputole32(le32tocpu(cnode->inode.blocks) + e->blocksToSecs(1));
			added = true;
		}

		cblock = e->blockCache.request(i,bmode);
		if(cblock == NULL)
			return 0;
		if(added) {
			memclear(cblock->buffer,e->blockSize());
			e->blockCache.markDirty(cblock);
		}
		if(req && Ext2INode::extend(e,cnode,cblock,block,&added) != 1) {
			e->blockCache.release(cblock);
			return 0;
		}
		i = le32tocpu(*((block_t*)(cblock->buffer) + block));
		e->blockCache.release(cblock);
		return i;
	}

	/* TODO we have to verify if this is all correct here... */

	/* doubly indirect */
	block -= blocksPerBlock;
	blperBlSq = blocksPerBlock * blocksPerBlock;
	if(block < blperBlSq) {
		added = false;
		i = le32tocpu(cnode->inode.doublyIBlock);
		/* no doubly-indirect-block present yet? */
		if(i == 0) {
			if(!req)
				return 0;
			i = Ext2Bitmap::allocBlock(e,cnode);
			cnode->inode.doublyIBlock = cputole32(i);
			if(i == 0)
				return 0;
			added = true;
			cnode->inode.blocks = cputole32(le32tocpu(cnode->inode.blocks) + e->blocksToSecs(1));
		}

		/* read the first block with block-numbers of the indirect blocks */
		cblock = e->blockCache.request(i,bmode);
		if(cblock == NULL)
			return 0;
		if(added) {
			memclear(cblock->buffer,e->blockSize());
			e->blockCache.markDirty(cblock);
		}
		if(req && Ext2INode::extend(e,cnode,cblock,block / blocksPerBlock,&added) != 1) {
			e->blockCache.release(cblock);
			return 0;
		}
		i = le32tocpu(*((block_t*)(cblock->buffer) + block / blocksPerBlock));
		e->blockCache.release(cblock);

		/* may happen if we should not request new blocks */
		if(i == 0)
			return 0;

		/* read the indirect block */
		cblock = e->blockCache.request(i,bmode);
		if(cblock == NULL)
			return 0;
		if(added) {
			memclear(cblock->buffer,e->blockSize());
			e->blockCache.markDirty(cblock);
		}
		if(req && Ext2INode::extend(e,cnode,cblock,block % blocksPerBlock,&added) != 1) {
			e->blockCache.release(cblock);
			return 0;
		}
		i = le32tocpu(*((block_t*)(cblock->buffer) + block % blocksPerBlock));
		e->blockCache.release(cblock);

		return i;
	}

	/* triply indirect */
	block -= blperBlSq;

	added = false;
	i = le32tocpu(cnode->inode.triplyIBlock);
	/* no triply-indirect-block present yet? */
	if(i == 0) {
		if(!req)
			return 0;
		i = Ext2Bitmap::allocBlock(e,cnode);
		cnode->inode.triplyIBlock = cputole32(i);
		if(i == 0)
			return 0;
		added = true;
		cnode->inode.blocks = cputole32(le32tocpu(cnode->inode.blocks) + e->blocksToSecs(1));
	}

	/* read the first block with block-numbers of the indirect blocks of indirect-blocks */
	cblock = e->blockCache.request(i,bmode);
	if(cblock == NULL)
		return 0;
	if(added) {
		memclear(cblock->buffer,e->blockSize());
		e->blockCache.markDirty(cblock);
	}
	if(req && Ext2INode::extend(e,cnode,cblock,block / blperBlSq,&added) != 1) {
		e->blockCache.release(cblock);
		return 0;
	}
	i = le32tocpu(*((block_t*)(cblock->buffer) + block / blperBlSq));
	e->blockCache.release(cblock);

	if(i == 0)
		return 0;

	/* read the indirect block of indirect blocks */
	block %= blperBlSq;
	cblock = e->blockCache.request(i,bmode);
	if(cblock == NULL)
		return 0;
	if(added) {
		memclear(cblock->buffer,e->blockSize());
		e->blockCache.markDirty(cblock);
	}
	if(req && Ext2INode::extend(e,cnode,cblock,block / blocksPerBlock,&added) != 1) {
		e->blockCache.release(cblock);
		return 0;
	}
	i = le32tocpu(*((block_t*)(cblock->buffer) + block / blocksPerBlock));
	e->blockCache.release(cblock);

	if(i == 0)
		return 0;

	/* read the indirect block */
	cblock = e->blockCache.request(i,bmode);
	if(cblock == NULL)
		return 0;
	if(added) {
		memclear(cblock->buffer,e->blockSize());
		e->blockCache.markDirty(cblock);
	}
	if(req && Ext2INode::extend(e,cnode,cblock,block % blocksPerBlock,&added) != 1) {
		e->blockCache.release(cblock);
		return 0;
	}
	i = le32tocpu(*((block_t*)(cblock->buffer) + block % blocksPerBlock));
	e->blockCache.release(cblock);

	return i;
}

int Ext2INode::extend(Ext2FileSystem *e,Ext2CInode *cnode,CBlock *cblock,size_t index,bool *added) {
	block_t *blockNos = (block_t*)(cblock->buffer);
	if(le32tocpu(blockNos[index]) == 0) {
		block_t bno = Ext2Bitmap::allocBlock(e,cnode);
		if(bno == 0)
			return 0;
		blockNos[index] = cputole32(bno);
		cnode->inode.blocks = cputole32(le32tocpu(cnode->inode.blocks) + e->blocksToSecs(1));
		e->blockCache.markDirty(cblock);
		*added = true;
	}
	else
		*added = false;
	return 1;
}

#if DEBUGGING

void Ext2INode::print(Ext2Inode *inode) {
	size_t i;
	printf("\tmode=0x%08x\n",le16tocpu(inode->mode));
	printf("\tuid=%d\n",le16tocpu(inode->uid));
	printf("\tgid=%d\n",le16tocpu(inode->gid));
	printf("\tsize=%d\n",le32tocpu(inode->size));
	printf("\taccesstime=%d\n",le32tocpu(inode->accesstime));
	printf("\tcreatetime=%d\n",le32tocpu(inode->createtime));
	printf("\tmodifytime=%d\n",le32tocpu(inode->modifytime));
	printf("\tdeletetime=%d\n",le32tocpu(inode->deletetime));
	printf("\tlinkCount=%d\n",le16tocpu(inode->linkCount));
	printf("\tblocks=%d\n",le32tocpu(inode->blocks));
	printf("\tflags=0x%08x\n",le32tocpu(inode->flags));
	printf("\tosd1=0x%08x\n",le32tocpu(inode->osd1));
	for(i = 0; i < EXT2_DIRBLOCK_COUNT; i++)
		printf("\tblock%zu=%d\n",i,le32tocpu(inode->dBlocks[i]));
	printf("\tsinglyIBlock=%d\n",le32tocpu(inode->singlyIBlock));
	printf("\tdoublyIBlock=%d\n",le32tocpu(inode->doublyIBlock));
	printf("\ttriplyIBlock=%d\n",le32tocpu(inode->triplyIBlock));
	printf("\tgeneration=%d\n",le32tocpu(inode->generation));
	printf("\tfileACL=%d\n",le32tocpu(inode->fileACL));
	printf("\tdirACL=%d\n",le32tocpu(inode->dirACL));
	printf("\tfragAddr=%d\n",le32tocpu(inode->fragAddr));
	printf("\tosd2=0x%08x%08x%08x%08x\n",le16tocpu(inode->osd2[0]),le16tocpu(inode->osd2[1]),
			le16tocpu(inode->osd2[2]),le16tocpu(inode->osd2[3]));
}

#endif
