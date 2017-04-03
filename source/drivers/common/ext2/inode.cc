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
#include <fs/permissions.h>
#include <sys/common.h>
#include <sys/endian.h>
#include <sys/io.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "bitmap.h"
#include "ext2.h"
#include "inode.h"
#include "inodecache.h"
#include "sbmng.h"

using namespace fs;

int Ext2INode::create(Ext2FileSystem *e,User *u,Ext2CInode *dirNode,Ext2CInode **ino,mode_t mode) {
	size_t i;
	time_t now;
	Ext2CInode *cnode;

	/* request inode */
	ino_t inodeNo = Ext2Bitmap::allocInode(e,dirNode,S_ISDIR(mode));
	if(inodeNo == 0)
		return -ENOSPC;
	cnode = e->inodeCache.request(inodeNo,IMODE_WRITE);
	if(cnode == NULL) {
		Ext2Bitmap::freeInode(e,inodeNo,S_ISDIR(mode));
		return -ENOBUFS;
	}

	/* init inode */
	cnode->inode.uid = cputole16(u->uid);
	cnode->inode.gid = cputole16(u->gid);
	cnode->inode.mode = cputole16(mode);
	cnode->inode.linkCount = cputole16(0);
	cnode->inode.size = cputole32(0);
	cnode->inode.singlyIBlock = cputole32(0);
	cnode->inode.doublyIBlock = cputole32(0);
	cnode->inode.triplyIBlock = cputole32(0);
	for(i = 0; i < EXT2_DIRBLOCK_COUNT; i++)
		cnode->inode.dBlocks[i] = cputole32(0);
	cnode->inode.blocks = cputole32(0);
	now = cputole32(time(NULL));
	cnode->inode.accesstime = now;
	cnode->inode.createtime = now;
	cnode->inode.modifytime = now;
	cnode->inode.deletetime = cputole32(0);
	e->inodeCache.markDirty(cnode);
	*ino = cnode;
	return 0;
}

int Ext2INode::chmod(Ext2FileSystem *e,User *u,ino_t inodeNo,mode_t mode) {
	mode_t oldMode;
	Ext2CInode *cnode = e->inodeCache.request(inodeNo,IMODE_WRITE);
	if(cnode == NULL)
		return -ENOBUFS;

	if(!Permissions::canChmod(u,le16tocpu(cnode->inode.uid)))
		return -EPERM;
	if(S_ISLNK(le16tocpu(cnode->inode.mode)))
		return -ENOTSUP;

	oldMode = le16tocpu(cnode->inode.mode);
	cnode->inode.mode = cputole16((oldMode & ~EXT2_S_PERMS) | (mode & EXT2_S_PERMS));
	e->inodeCache.markDirty(cnode);
	e->inodeCache.release(cnode);
	return 0;
}

int Ext2INode::chown(Ext2FileSystem *e,User *u,ino_t inodeNo,uid_t uid,gid_t gid) {
	uid_t oldUid;
	gid_t oldGid;
	Ext2CInode *cnode = e->inodeCache.request(inodeNo,IMODE_WRITE);
	if(cnode == NULL)
		return -ENOBUFS;

	oldUid = le16tocpu(cnode->inode.uid);
	oldGid = le16tocpu(cnode->inode.gid);
	if(!Permissions::canChown(u,oldUid,oldGid,uid,gid))
		return -EPERM;
	if(S_ISLNK(le16tocpu(cnode->inode.mode)))
		return -ENOTSUP;

	if(uid != (uid_t)-1)
		cnode->inode.uid = cputole16(uid);
	if(gid != (gid_t)-1)
		cnode->inode.gid = cputole16(gid);
	e->inodeCache.markDirty(cnode);
	e->inodeCache.release(cnode);
	return 0;
}

int Ext2INode::utime(Ext2FileSystem *e,User *u,ino_t inodeNo,const struct utimbuf *utimes) {
	Ext2CInode *cnode = e->inodeCache.request(inodeNo,IMODE_WRITE);
	if(cnode == NULL)
		return -ENOBUFS;

	if(!Permissions::canUtime(u,le16tocpu(cnode->inode.uid)))
		return -EPERM;

	cnode->inode.accesstime = cputole32(utimes->actime);
	cnode->inode.modifytime = cputole32(utimes->modtime);
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
	cnode->inode.deletetime = cputole32(time(NULL));
	cnode->inode.linkCount = cputole16(0);
	e->inodeCache.markDirty(cnode);
	return 0;
}

block_t Ext2INode::accessIndirBlock(Ext2FileSystem *e,Ext2CInode *cnode,block_t *indir,block_t i,
		bool req,int level,block_t div) {
	bool added = false;
	uint bmode = req ? BlockCache::WRITE : BlockCache::READ;
	size_t blockSize = e->blockSize();
	size_t blocksPerBlock = blockSize / sizeof(block_t);
	CBlock *cblock;

	/* is the pointer to the block we want to write the block number into still missing? */
	if(*indir == 0) {
		if(!req)
			return 0;
		*indir = cputole32(Ext2Bitmap::allocBlock(e,cnode));
		if(!*indir)
			return 0;
		added = true;
	}

	/* request the block with block numbers */
	cblock = e->blockCache.request(le32tocpu(*indir),bmode);
	if(cblock == NULL)
		return 0;
	/* clear it, if we just created it */
	if(added)
		memclear(cblock->buffer,blockSize);

	block_t bno = 0;
	block_t *blockNos = (block_t*)(cblock->buffer);
	/* if we're the last level, write the block-number into it */
	if(level == 0) {
		assert(i < blocksPerBlock);
		if(blockNos[i] == 0) {
			if(!req)
				goto error;

			blockNos[i] = cputole32(Ext2Bitmap::allocBlock(e,cnode));
			if(blockNos[i] == 0)
				goto error;

			cnode->inode.blocks = cputole32(le32tocpu(cnode->inode.blocks) + e->blocksToSecs(1));
			e->blockCache.markDirty(cblock);
		}
		bno = le32tocpu(blockNos[i]);
	}
	/* otherwise let the callee write the block-number into cblock */
	else {
		block_t *subIndir = blockNos + i / div;
		/* mark the block dirty, if the callee will write to it */
		if(req && !*subIndir)
			e->blockCache.markDirty(cblock);
		bno = accessIndirBlock(e,cnode,subIndir,i % div,req,level - 1,div / blocksPerBlock);
	}

error:
	e->blockCache.release(cblock);
	return bno;
}

block_t Ext2INode::doGetDataBlock(Ext2FileSystem *e,Ext2CInode *cnode,block_t block,bool req) {
	size_t blockSize = e->blockSize();
	size_t blocksPerBlock = blockSize / sizeof(block_t);

	if(block < EXT2_DIRBLOCK_COUNT) {
		block_t bno = le32tocpu(cnode->inode.dBlocks[block]);
		if(req && bno == 0) {
			bno = Ext2Bitmap::allocBlock(e,cnode);
			cnode->inode.dBlocks[block] = cputole32(bno);
			if(bno != 0) {
				uint32_t blocks = le32tocpu(cnode->inode.blocks);
				cnode->inode.blocks = cputole32(blocks + e->blocksToSecs(1));
			}
		}
		return bno;
	}

	block -= EXT2_DIRBLOCK_COUNT;
	if(block < blocksPerBlock)
		return accessIndirBlock(e,cnode,&cnode->inode.singlyIBlock,block,req,0,1);

	block -= blocksPerBlock;
	if(block < blocksPerBlock * blocksPerBlock)
		return accessIndirBlock(e,cnode,&cnode->inode.doublyIBlock,block,req,1,blocksPerBlock);

	block -= blocksPerBlock * blocksPerBlock;
	if(block < blocksPerBlock * blocksPerBlock * blocksPerBlock) {
		return accessIndirBlock(e,cnode,&cnode->inode.triplyIBlock,block,req,2,
			blocksPerBlock * blocksPerBlock);
	}

	/* too large */
	return 0;
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
