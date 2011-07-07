/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <esc/endian.h>
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
#include "inode.h"
#include "dir.h"
#include "rw.h"

static inode_t ext2_resPath(void *h,sFSUser *u,const char *path,uint flags,dev_t *dev,bool resLastMnt);
static inode_t ext2_open(void *h,sFSUser *u,inode_t ino,uint flags);
static void ext2_close(void *h,inode_t ino);
static int ext2_stat(void *h,inode_t ino,sFileInfo *info);
static int ext2_chmod(void *h,sFSUser *u,inode_t inodeNo,mode_t mode);
static int ext2_chown(void *h,sFSUser *u,inode_t inodeNo,uid_t uid,gid_t gid);
static ssize_t ext2_read(void *h,inode_t inodeNo,void *buffer,off_t offset,size_t count);
static ssize_t ext2_write(void *h,inode_t inodeNo,const void *buffer,off_t offset,size_t count);
static int ext2_link(void *h,sFSUser *u,inode_t dstIno,inode_t dirIno,const char *name);
static int ext2_unlink(void *h,sFSUser *u,inode_t dirIno,const char *name);
static int ext2_mkdir(void *h,sFSUser *u,inode_t dirIno,const char *name);
static int ext2_rmdir(void *h,sFSUser *u,inode_t dirIno,const char *name);
static void ext2_sync(void *h);
static bool ext2_isPowerOf(uint x,uint y);

void *ext2_init(const char *driver,char **usedDev) {
	size_t i;
	sExt2 *e = (sExt2*)calloc(1,sizeof(sExt2));
	if(e == NULL)
		return NULL;
	for(i = 0; i <= REQ_THREAD_COUNT; i++)
		e->drvFds[i] = -1;
	e->blockCache.blockCache = NULL;

	/* open the driver */
	for(i = 0; i <= REQ_THREAD_COUNT; i++) {
		e->drvFds[i] = open(driver,IO_WRITE | IO_READ);
		if(e->drvFds[i] < 0) {
			printe("Unable to find driver '%s'",driver);
			goto error;
		}
	}

	if(!ext2_super_init(e))
		goto error;

	/* Note: we don't need it in ext2_super_init() and we can't use EXT2_BLK_SIZE() without
	 * super-block! */
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
	if(!ext2_bg_init(e))
		goto error;

	/* init caches */
	ext2_icache_init(e);
	bcache_init(&e->blockCache);

	/* report used driver */
	*usedDev = malloc(strlen(driver) + 1);
	if(!*usedDev) {
		printe("Not enough mem for driver-name");
		goto error;
	}
	strcpy(*usedDev,driver);
	return e;

error:
	if(e->blockCache.blockCache)
		bcache_destroy(&e->blockCache);
	for(i = 0; i <= REQ_THREAD_COUNT; i++) {
		if(e->drvFds[i] >= 0) {
			close(e->drvFds[i]);
			e->drvFds[i] = -1;
		}
	}
	free(e);
	return NULL;
}

void ext2_deinit(void *h) {
	size_t i;
	sExt2 *e = (sExt2*)h;
	/* write pending changes */
	ext2_sync(e);
	/* clean up */
	ext2_bg_destroy(e);
	bcache_destroy(&e->blockCache);
	for(i = 0; i <= REQ_THREAD_COUNT; i++) {
		if(e->drvFds[i] >= 0) {
			close(e->drvFds[i]);
			e->drvFds[i] = -1;
		}
	}
	free(e);
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
	fs->chmod = ext2_chmod;
	fs->chown = ext2_chown;
	fs->sync = ext2_sync;
	return fs;
}

int ext2_hasPermission(sExt2CInode *cnode,sFSUser *u,uint perms) {
	int mask;
	uint16_t mode = le16tocpu(cnode->inode.mode);
	if(u->uid == ROOT_UID) {
		/* root has exec-permission if at least one has exec-permission */
		if(perms & MODE_EXEC)
			return (mode & MODE_EXEC) ? 0 : ERR_NO_EXEC_PERM;
		/* root can read and write in all cases */
		return 0;
	}

	if(le16tocpu(cnode->inode.uid) == u->uid)
		mask = (mode >> 6) & 0x7;
	else if(le16tocpu(cnode->inode.gid) == u->gid || isingroup(u->pid,le16tocpu(cnode->inode.gid)) == 1)
		mask = (mode >> 3) & 0x7;
	else
		mask = mode & 0x7;

	if((perms & MODE_READ) && !(mask & MODE_READ))
		return ERR_NO_READ_PERM;
	if((perms & MODE_WRITE) && !(mask & MODE_WRITE))
		return ERR_NO_WRITE_PERM;
	if((perms & MODE_EXEC) && !(mask & MODE_EXEC))
		return ERR_NO_EXEC_PERM;
	return 0;
}

block_t ext2_getBlockOfInode(sExt2 *e,inode_t inodeNo) {
	return (inodeNo - 1) / le32tocpu(e->superBlock.inodesPerGroup);
}

block_t ext2_getGroupOfBlock(sExt2 *e,block_t block) {
	return block / le32tocpu(e->superBlock.blocksPerGroup);
}

block_t ext2_getGroupOfInode(sExt2 *e,inode_t inodeNo) {
	return inodeNo / le32tocpu(e->superBlock.inodesPerGroup);
}

size_t ext2_getBlockGroupCount(sExt2 *e) {
	size_t bpg = le32tocpu(e->superBlock.blocksPerGroup);
	return (le32tocpu(e->superBlock.blockCount) + bpg - 1) / bpg;
}

bool ext2_bgHasBackups(sExt2 *e,block_t i) {
	/* if the sparse-feature is enabled, just the groups 0, 1 and powers of 3, 5 and 7 contain
	 * the backup */
	if(!(le32tocpu(e->superBlock.featureRoCompat) & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER))
		return true;
	/* block-group 0 is handled manually */
	if(i == 1)
		return true;
	return ext2_isPowerOf(i,3) || ext2_isPowerOf(i,5) || ext2_isPowerOf(i,7);
}

static inode_t ext2_resPath(void *h,sFSUser *u,const char *path,uint flags,dev_t *dev,
		bool resLastMnt) {
	return ext2_path_resolve((sExt2*)h,u,path,flags,dev,resLastMnt);
}

static inode_t ext2_open(void *h,sFSUser *u,inode_t ino,uint flags) {
	int err;
	sExt2 *e = (sExt2*)h;
	/* check permissions */
	sExt2CInode *cnode = ext2_icache_request(e,ino,IMODE_READ);
	uint mode = 0;
	if(flags & IO_READ)
		mode |= MODE_READ;
	if(flags & IO_WRITE)
		mode |= MODE_WRITE;
	/* TODO exec? */
	if((err = ext2_hasPermission(cnode,u,mode)) < 0)
		return err;
	ext2_icache_release(cnode);

	/* truncate? */
	if(flags & IO_TRUNCATE) {
		cnode = ext2_icache_request(e,ino,IMODE_WRITE);
		if(cnode != NULL) {
			ext2_file_truncate(e,cnode,false);
			ext2_icache_release(cnode);
		}
	}
	return ino;
}

static void ext2_close(void *h,inode_t ino) {
	UNUSED(h);
	UNUSED(ino);
}

static int ext2_stat(void *h,inode_t ino,sFileInfo *info) {
	sExt2 *e = (sExt2*)h;
	const sExt2CInode *cnode = ext2_icache_request(e,ino,IMODE_READ);
	if(cnode == NULL)
		return ERR_INO_REQ_FAILED;

	info->accesstime = le32tocpu(cnode->inode.accesstime);
	info->modifytime = le32tocpu(cnode->inode.modifytime);
	info->createtime = le32tocpu(cnode->inode.createtime);
	info->blockCount = le32tocpu(cnode->inode.blocks);
	info->blockSize = EXT2_BLK_SIZE(e);
	info->device = mount_getDevByHandle(h);
	info->uid = le16tocpu(cnode->inode.uid);
	info->gid = le16tocpu(cnode->inode.gid);
	info->inodeNo = cnode->inodeNo;
	info->linkCount = le16tocpu(cnode->inode.linkCount);
	info->mode = le16tocpu(cnode->inode.mode);
	info->size = le32tocpu(cnode->inode.size);
	ext2_icache_release(cnode);
	return 0;
}

static int ext2_chmod(void *h,sFSUser *u,inode_t inodeNo,mode_t mode) {
	return ext2_inode_chmod((sExt2*)h,u,inodeNo,mode);
}

static int ext2_chown(void *h,sFSUser *u,inode_t inodeNo,uid_t uid,gid_t gid) {
	return ext2_inode_chown((sExt2*)h,u,inodeNo,uid,gid);
}

static ssize_t ext2_read(void *h,inode_t inodeNo,void *buffer,off_t offset,size_t count) {
	return ext2_file_read((sExt2*)h,inodeNo,buffer,offset,count);
}

static ssize_t ext2_write(void *h,inode_t inodeNo,const void *buffer,off_t offset,size_t count) {
	return ext2_file_write((sExt2*)h,inodeNo,buffer,offset,count);
}

static int ext2_link(void *h,sFSUser *u,inode_t dstIno,inode_t dirIno,const char *name) {
	sExt2 *e = (sExt2*)h;
	int res;
	sExt2CInode *dir,*ino;
	dir = ext2_icache_request(e,dirIno,IMODE_WRITE);
	ino = ext2_icache_request(e,dstIno,IMODE_WRITE);
	if(dir == NULL || ino == NULL)
		res = ERR_INO_REQ_FAILED;
	else if(S_ISDIR(le16tocpu(ino->inode.mode)))
		res = ERR_IS_DIR;
	else
		res = ext2_link_create(e,u,dir,ino,name);
	ext2_icache_release(dir);
	ext2_icache_release(ino);
	return res;
}

static int ext2_unlink(void *h,sFSUser *u,inode_t dirIno,const char *name) {
	sExt2 *e = (sExt2*)h;
	int res;
	sExt2CInode *dir = ext2_icache_request(e,dirIno,IMODE_WRITE);
	if(dir == NULL)
		return ERR_INO_REQ_FAILED;

	res = ext2_link_delete(e,u,NULL,dir,name,false);
	ext2_icache_release(dir);
	return res;
}

static int ext2_mkdir(void *h,sFSUser *u,inode_t dirIno,const char *name) {
	sExt2 *e = (sExt2*)h;
	int res;
	sExt2CInode *dir = ext2_icache_request(e,dirIno,IMODE_WRITE);
	if(dir == NULL)
		return ERR_INO_REQ_FAILED;
	res = ext2_dir_create(e,u,dir,name);
	ext2_icache_release(dir);
	return res;
}

static int ext2_rmdir(void *h,sFSUser *u,inode_t dirIno,const char *name) {
	sExt2 *e = (sExt2*)h;
	int res;
	sExt2CInode *dir = ext2_icache_request(e,dirIno,IMODE_WRITE);
	if(dir == NULL)
		return ERR_INO_REQ_FAILED;
	if(!S_ISDIR(le16tocpu(dir->inode.mode)))
		return ERR_NO_DIRECTORY;
	res = ext2_dir_delete(e,u,dir,name);
	ext2_icache_release(dir);
	return res;
}

static void ext2_sync(void *h) {
	sExt2 *e = (sExt2*)h;
	ext2_super_update(e);
	ext2_bg_update(e);
	/* flush inodes first, because they may create dirty blocks */
	ext2_icache_flush(e);
	bcache_flush(&e->blockCache);
}

static bool ext2_isPowerOf(uint x,uint y) {
	while(x > 1) {
		if(x % y != 0)
			return false;
		x /= y;
	}
	return true;
}

#if DEBUGGING

void ext2_printBlockGroups(sExt2 *e) {
	size_t i,count = ext2_getBlockGroupCount(e);
	printf("Blockgroups:\n");
	for(i = 0; i < count; i++) {
		printf(" Block %d\n",i);
		ext2_bg_print(e,i,e->groups + i);
	}
}

#endif
