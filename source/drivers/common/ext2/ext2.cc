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
#include <esc/proc.h>
#include <esc/debug.h>
#include <esc/dir.h>
#include <esc/endian.h>
#include <fs/fsdev.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ext2.h"
#include "inodecache.h"
#include "bgmng.h"
#include "sbmng.h"
#include "path.h"
#include "file.h"
#include "link.h"
#include "inode.h"
#include "dir.h"
#include "rw.h"

int main(int argc,char *argv[]) {
	char fspath[MAX_PATH_LEN];
	if(argc != 3)
		error("Usage: %s <wait> <devicePath>",argv[0]);

	/* the backend has to be a block device */
	if(!isblock(argv[2]))
		error("'%s' is neither a block-device nor a regular file",argv[2]);

	/* build fs device name */
	char *dev = strrchr(argv[2],'/');
	if(!dev)
		dev = argv[2] - 1;
	snprintf(fspath,sizeof(fspath),"/dev/ext2-%s",dev + 1);

	FSDevice fsdev(new Ext2FileSystem(argv[2]),fspath);
	fsdev.loop();
	return 0;
}

bool Ext2FileSystem::Ext2BlockCache::readBlocks(void *buffer,block_t start,size_t blockCount) {
	return Ext2RW::readBlocks(_fs,buffer,start,blockCount);
}

bool Ext2FileSystem::Ext2BlockCache::writeBlocks(const void *buffer,size_t start,size_t blockCount) {
	return Ext2RW::writeBlocks(_fs,buffer,start,blockCount);
}

Ext2FileSystem::Ext2FileSystem(const char *device)
		: fd(::open(device,O_RDWR)), sb(this), bgs(this),
		  inodeCache(this), blockCache(this) {
	if(fd < 0)
		VTHROWE("Unable to open device '" << device << "'",fd);
}

Ext2FileSystem::~Ext2FileSystem() {
	/* write pending changes */
	sync();
	close(fd);
}

inode_t Ext2FileSystem::resolve(FSUser *u,const char *path,uint flags) {
	return Ext2Path::resolve(this,u,path,flags);
}

inode_t Ext2FileSystem::open(FSUser *u,inode_t ino,uint flags) {
	int err;
	/* check permissions */
	Ext2CInode *cnode = inodeCache.request(ino,IMODE_READ);
	uint mode = 0;
	if(flags & O_READ)
		mode |= MODE_READ;
	if(flags & O_WRITE)
		mode |= MODE_WRITE;
	/* TODO exec? */
	if((err = hasPermission(cnode,u,mode)) < 0) {
		inodeCache.release(cnode);
		return err;
	}
	/* increase references so that the inode stays in cache until we close it. this is necessary
	 * to prevent that somebody else deletes the file while another one uses it. of course, this
	 * means that we can never have more open files that inode-cache-slots. so, we might have to
	 * increase that at sometime. */
	cnode->refs++;
	inodeCache.release(cnode);

	/* truncate? */
	if(flags & O_TRUNC) {
		cnode = inodeCache.request(ino,IMODE_WRITE);
		if(cnode != NULL) {
			Ext2File::truncate(this,cnode,false);
			inodeCache.release(cnode);
		}
	}
	return ino;
}

void Ext2FileSystem::close(inode_t ino) {
	/* decrease references so that we can remove the cached inode and maybe even delete the file */
	Ext2CInode *cnode = inodeCache.request(ino,IMODE_READ);
	cnode->refs--;
	inodeCache.release(cnode);
}

int Ext2FileSystem::stat(inode_t ino,sFileInfo *info) {
	const Ext2CInode *cnode = inodeCache.request(ino,IMODE_READ);
	if(cnode == NULL)
		return -ENOBUFS;

	info->accesstime = le32tocpu(cnode->inode.accesstime);
	info->modifytime = le32tocpu(cnode->inode.modifytime);
	info->createtime = le32tocpu(cnode->inode.createtime);
	info->blockCount = le32tocpu(cnode->inode.blocks);
	info->blockSize = blockSize();
	info->device = 0;
	info->uid = le16tocpu(cnode->inode.uid);
	info->gid = le16tocpu(cnode->inode.gid);
	info->inodeNo = cnode->inodeNo;
	info->linkCount = le16tocpu(cnode->inode.linkCount);
	info->mode = le16tocpu(cnode->inode.mode);
	info->size = le32tocpu(cnode->inode.size);
	inodeCache.release(cnode);
	return 0;
}

int Ext2FileSystem::chmod(FSUser *u,inode_t inodeNo,mode_t mode) {
	return Ext2INode::chmod(this,u,inodeNo,mode);
}

int Ext2FileSystem::chown(FSUser *u,inode_t inodeNo,uid_t uid,gid_t gid) {
	return Ext2INode::chown(this,u,inodeNo,uid,gid);
}

ssize_t Ext2FileSystem::read(inode_t inodeNo,void *buffer,off_t offset,size_t count) {
	return Ext2File::read(this,inodeNo,buffer,offset,count);
}

ssize_t Ext2FileSystem::write(inode_t inodeNo,const void *buffer,off_t offset,size_t count) {
	return Ext2File::write(this,inodeNo,buffer,offset,count);
}

int Ext2FileSystem::link(FSUser *u,inode_t dstIno,inode_t dirIno,const char *name) {
	int res;
	Ext2CInode *dir,*ino;
	dir = inodeCache.request(dirIno,IMODE_WRITE);
	ino = inodeCache.request(dstIno,IMODE_WRITE);
	if(dir == NULL || ino == NULL)
		res = -ENOBUFS;
	else if(S_ISDIR(le16tocpu(ino->inode.mode)))
		res = -EISDIR;
	else
		res = Ext2Link::create(this,u,dir,ino,name);
	inodeCache.release(dir);
	inodeCache.release(ino);
	return res;
}

int Ext2FileSystem::unlink(FSUser *u,inode_t dirIno,const char *name) {
	int res;
	Ext2CInode *dir = inodeCache.request(dirIno,IMODE_WRITE);
	if(dir == NULL)
		return -ENOBUFS;

	res = Ext2Link::remove(this,u,NULL,dir,name,false);
	inodeCache.release(dir);
	return res;
}

int Ext2FileSystem::mkdir(FSUser *u,inode_t dirIno,const char *name) {
	int res;
	Ext2CInode *dir = inodeCache.request(dirIno,IMODE_WRITE);
	if(dir == NULL)
		return -ENOBUFS;
	res = Ext2Dir::create(this,u,dir,name);
	inodeCache.release(dir);
	return res;
}

int Ext2FileSystem::rmdir(FSUser *u,inode_t dirIno,const char *name) {
	int res;
	Ext2CInode *dir = inodeCache.request(dirIno,IMODE_WRITE);
	if(dir == NULL)
		return -ENOBUFS;
	if(!S_ISDIR(le16tocpu(dir->inode.mode)))
		return -ENOTDIR;
	res = Ext2Dir::remove(this,u,dir,name);
	inodeCache.release(dir);
	return res;
}

void Ext2FileSystem::sync() {
	sb.update();
	bgs.update();
	/* flush inodes first, because they may create dirty blocks */
	inodeCache.flush();
	blockCache.flush();
}

void Ext2FileSystem::print(FILE *f) {
	fprintf(f,"Total blocks: %u\n",le32tocpu(sb.get()->blockCount));
	fprintf(f,"Total inodes: %u\n",le32tocpu(sb.get()->inodeCount));
	fprintf(f,"Free blocks: %u\n",le32tocpu(sb.get()->freeBlockCount));
	fprintf(f,"Free inodes: %u\n",le32tocpu(sb.get()->freeInodeCount));
	fprintf(f,"Blocks per group: %u\n",le32tocpu(sb.get()->blocksPerGroup));
	fprintf(f,"Inodes per group: %u\n",le32tocpu(sb.get()->inodesPerGroup));
	fprintf(f,"Blocksize: %zu\n",blockSize());
	fprintf(f,"Capacity: %zu bytes\n",le32tocpu(sb.get()->blockCount) * blockSize());
	fprintf(f,"Free: %zu bytes\n",le32tocpu(sb.get()->freeBlockCount) * blockSize());
	fprintf(f,"Mount count: %u\n",le16tocpu(sb.get()->mountCount));
	fprintf(f,"Max mount count: %u\n",le16tocpu(sb.get()->maxMountCount));
	fprintf(f,"Block cache:\n");
	blockCache.printStats(f);
	fprintf(f,"Inode cache:\n");
	inodeCache.print(f);
}

int Ext2FileSystem::hasPermission(Ext2CInode *cnode,FSUser *u,uint perms) {
	int mask;
	uint16_t mode = le16tocpu(cnode->inode.mode);
	if(u->uid == ROOT_UID) {
		/* root has exec-permission if at least one has exec-permission */
		if(perms & MODE_EXEC)
			return (mode & MODE_EXEC) ? 0 : -EACCES;
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
		return -EACCES;
	if((perms & MODE_WRITE) && !(mask & MODE_WRITE))
		return -EACCES;
	if((perms & MODE_EXEC) && !(mask & MODE_EXEC))
		return -EACCES;
	return 0;
}

bool Ext2FileSystem::bgHasBackups(block_t i) {
	/* if the sparse-feature is enabled, just the groups 0, 1 and powers of 3, 5 and 7 contain
	 * the backup */
	if(!(le32tocpu(sb.get()->featureRoCompat) & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER))
		return true;
	/* block-group 0 is handled manually */
	if(i == 1)
		return true;
	return isPowerOf(i,3) || isPowerOf(i,5) || isPowerOf(i,7);
}

#if DEBUGGING

void Ext2FileSystem::printBlockGroups() {
	size_t i,count = getBlockGroupCount();
	printf("Blockgroups:\n");
	for(i = 0; i < count; i++) {
		printf(" Block %zu\n",i);
		bgs.print(i);
	}
}

#endif
