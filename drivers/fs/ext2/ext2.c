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
#include <esc/proc.h>
#include <esc/debug.h>
#include <esc/dir.h>
#include <errors.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ext2.h"
#include "../blockcache.h"
#include "inodecache.h"
#include "blockgroup.h"
#include "superblock.h"
#include "path.h"
#include "file.h"
#include "link.h"
#include "dir.h"
#include "rw.h"

/**
 * Checks whether x is a power of y
 */
static bool ext2_isPowerOf(u32 x,u32 y);

void *ext2_init(const char *driver,char **usedDev) {
	tFD fd;
	sExt2 *e = (sExt2*)calloc(1,sizeof(sExt2));
	if(e == NULL)
		return NULL;

	/* open the driver */
	fd = open(driver,IO_WRITE | IO_READ);
	if(fd < 0) {
		printe("Unable to find driver '%s'",driver);
		return NULL;
	}

	e->ataFd = fd;
	if(!ext2_super_init(e)) {
		close(e->ataFd);
		free(e);
		return NULL;
	}

	/* Note: we don't need it in ext2_super_init() and we can't use EXT2_BLK_SIZE() without
	 * super-block! */
	e->blockCache.blockCache = NULL;
	e->blockCache.blockCacheSize = EXT2_BCACHE_SIZE;
	e->blockCache.blockSize = EXT2_BLK_SIZE(e);
	e->blockCache.freeBlocks = NULL;
	e->blockCache.usedBlocks = NULL;
	e->blockCache.oldestBlock = NULL;
	e->blockCache.handle = e;
	/* cast is ok, because the only difference is that ext2_rw_* receive a sExt2* as first argument
	 * and read/write expect void* */
	e->blockCache.read = (fReadBlocks)ext2_rw_readBlocks;
	e->blockCache.write = (fWriteBlocks)ext2_rw_writeBlocks;

	/* init block-groups */
	if(!ext2_bg_init(e)) {
		free(e);
		return NULL;
	}

	/* init caches */
	ext2_icache_init(e);
	bcache_init(&e->blockCache);

	/* report used driver */
	*usedDev = malloc(strlen(driver) + 1);
	if(!*usedDev) {
		printe("Not enough mem for driver-name");
		bcache_destroy(&e->blockCache);
		free(e);
		return NULL;
	}
	strcpy(*usedDev,driver);
	return e;
}

void ext2_deinit(void *h) {
	sExt2 *e = (sExt2*)h;
	ext2_sync(e);
}

sFileSystem *ext2_getFS(void) {
	sFileSystem *fs = malloc(sizeof(sFileSystem));
	if(!fs)
		return NULL;
	fs->type = FS_TYPE_EXT2;
	fs->init = ext2_init;
	fs->deinit = ext2_deinit;
	fs->resPath = ext2_resPath;
	fs->open = ext2_open;
	fs->close = ext2_close;
	fs->stat = ext2_stat;
	fs->read = ext2_read;
	fs->write = ext2_write;
	fs->link = ext2_link;
	fs->unlink = ext2_unlink;
	fs->mkdir = ext2_mkdir;
	fs->rmdir = ext2_rmdir;
	fs->sync = ext2_sync;
	return fs;
}

tInodeNo ext2_resPath(void *h,const char *path,u8 flags,tDevNo *dev,bool resLastMnt) {
	return ext2_path_resolve((sExt2*)h,path,flags,dev,resLastMnt);
}

s32 ext2_open(void *h,tInodeNo ino,u8 flags) {
	sExt2 *e = (sExt2*)h;
	/* truncate? */
	if(flags & IO_TRUNCATE) {
		sExt2CInode *cnode = ext2_icache_request(e,ino);
		if(cnode != NULL) {
			ext2_file_truncate(e,cnode,false);
			ext2_icache_release(e,cnode);
		}
	}
	/*ext2_icache_printStats();
	bcache_printStats();*/
	return ino;
}

void ext2_close(void *h,tInodeNo ino) {
	UNUSED(h);
	UNUSED(ino);
}

s32 ext2_stat(void *h,tInodeNo ino,sFileInfo *info) {
	sExt2 *e = (sExt2*)h;
	sExt2CInode *cnode = ext2_icache_request(e,ino);
	if(cnode == NULL)
		return ERR_INO_REQ_FAILED;

	info->accesstime = cnode->inode.accesstime;
	info->modifytime = cnode->inode.modifytime;
	info->createtime = cnode->inode.createtime;
	info->blockCount = cnode->inode.blocks;
	info->blockSize = EXT2_BLK_SIZE(e);
	info->device = mount_getDevByHandle(h);
	info->uid = cnode->inode.uid;
	info->gid = cnode->inode.gid;
	info->inodeNo = cnode->inodeNo;
	info->linkCount = cnode->inode.linkCount;
	info->mode = cnode->inode.mode;
	info->size = cnode->inode.size;
	ext2_icache_release(e,cnode);
	return 0;
}

s32 ext2_read(void *h,tInodeNo inodeNo,void *buffer,u32 offset,u32 count) {
	return ext2_file_read((sExt2*)h,inodeNo,buffer,offset,count);
}

s32 ext2_write(void *h,tInodeNo inodeNo,const void *buffer,u32 offset,u32 count) {
	return ext2_file_write((sExt2*)h,inodeNo,buffer,offset,count);
}

s32 ext2_link(void *h,tInodeNo dstIno,tInodeNo dirIno,const char *name) {
	sExt2 *e = (sExt2*)h;
	s32 res;
	sExt2CInode *dir,*ino;
	dir = ext2_icache_request(e,dirIno);
	ino = ext2_icache_request(e,dstIno);
	if(dir == NULL || ino == NULL)
		res = ERR_INO_REQ_FAILED;
	else if(MODE_IS_DIR(ino->inode.mode))
		res = ERR_IS_DIR;
	else
		res = ext2_link_create(e,dir,ino,name);
	ext2_icache_release(e,dir);
	ext2_icache_release(e,ino);
	return res;
}

s32 ext2_unlink(void *h,tInodeNo dirIno,const char *name) {
	sExt2 *e = (sExt2*)h;
	s32 res;
	sExt2CInode *dir = ext2_icache_request(e,dirIno);
	if(dir == NULL)
		return ERR_INO_REQ_FAILED;

	res = ext2_link_delete(e,dir,name,false);
	ext2_icache_release(e,dir);
	return res;
}

s32 ext2_mkdir(void *h,tInodeNo dirIno,const char *name) {
	sExt2 *e = (sExt2*)h;
	s32 res;
	sExt2CInode *dir = ext2_icache_request(e,dirIno);
	if(dir == NULL)
		return ERR_INO_REQ_FAILED;
	res = ext2_dir_create(e,dir,name);
	ext2_icache_release(e,dir);
	return res;
}

s32 ext2_rmdir(void *h,tInodeNo dirIno,const char *name) {
	sExt2 *e = (sExt2*)h;
	s32 res;
	sExt2CInode *dir = ext2_icache_request(e,dirIno);
	if(dir == NULL)
		return ERR_INO_REQ_FAILED;
	if(!MODE_IS_DIR(dir->inode.mode))
		return ERR_NO_DIRECTORY;
	res = ext2_dir_delete(e,dir,name);
	ext2_icache_release(e,dir);
	return res;
}

void ext2_sync(void *h) {
	sExt2 *e = (sExt2*)h;
	ext2_super_update(e);
	ext2_bg_update(e);
	/* flush inodes first, because they may create dirty blocks */
	ext2_icache_flush(e);
	bcache_flush(&e->blockCache);
}

u32 ext2_getBlockOfInode(sExt2 *e,tInodeNo inodeNo) {
	return (inodeNo - 1) / e->superBlock.inodesPerGroup;
}

u32 ext2_getGroupOfBlock(sExt2 *e,u32 block) {
	return block / e->superBlock.blocksPerGroup;
}

u32 ext2_getGroupOfInode(sExt2 *e,tInodeNo inodeNo) {
	return inodeNo / e->superBlock.inodesPerGroup;
}

u32 ext2_getBlockGroupCount(sExt2 *e) {
	u32 bpg = e->superBlock.blocksPerGroup;
	return (e->superBlock.blockCount + bpg - 1) / bpg;
}

bool ext2_bgHasBackups(sExt2 *e,u32 i) {
	/* if the sparse-feature is enabled, just the groups 0, 1 and powers of 3, 5 and 7 contain
	 * the backup */
	if(!(e->superBlock.featureRoCompat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER))
		return true;
	/* block-group 0 is handled manually */
	if(i == 1)
		return true;
	return ext2_isPowerOf(i,3) || ext2_isPowerOf(i,5) || ext2_isPowerOf(i,7);
}

void ext2_destroy(sExt2 *e) {
	free(e->groups);
	close(e->ataFd);
}

static bool ext2_isPowerOf(u32 x,u32 y) {
	while(x > 1) {
		if(x % y != 0)
			return false;
		x /= y;
	}
	return true;
}

#if DEBUGGING

void ext2_bg_prints(sExt2 *e) {
	u32 i,count = ext2_getBlockGroupCount(e);
	printf("Blockgroups:\n");
	for(i = 0; i < count; i++) {
		printf(" Block %d\n",i);
		ext2_bg_print(e,i,e->groups + i);
	}
}

#endif
