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

#include <fs/fsdev.h>
#include <fs/permissions.h>
#include <sys/common.h>
#include <sys/debug.h>
#include <sys/endian.h>
#include <sys/io.h>
#include <sys/proc.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bgmng.h"
#include "dir.h"
#include "ext2.h"
#include "file.h"
#include "inode.h"
#include "inodecache.h"
#include "link.h"
#include "path.h"
#include "rw.h"
#include "sbmng.h"

static fs::FSDevice<fs::OpenFile> *fsdev;

static void sigTermHndl(int) {
	/* notify init that we're alive and promise to terminate as soon as possible */
	esc::Init init("/dev/init");
	init.iamalive();
	fsdev->stop();
}

int main(int argc,char *argv[]) {
	if(argc != 3)
		error("Usage: %s <fsPath> <devicePath>",argv[0]);

	/* the backend has to be a block device */
	if(!isblock(argv[2]))
		error("'%s' is neither a block-device nor a regular file",argv[2]);

	if(signal(SIGTERM,sigTermHndl) == SIG_ERR)
		error("Unable to set signal-handler for SIGTERM");

	fsdev = new fs::FSDevice<fs::OpenFile>(new Ext2FileSystem(argv[2]),argv[1]);
	fsdev->loop();
	return 0;
}

bool Ext2FileSystem::Ext2BlockCache::readBlocks(void *buffer,block_t start,size_t blockCount) {
	return Ext2RW::readBlocks(_fs,buffer,start,blockCount);
}

bool Ext2FileSystem::Ext2BlockCache::writeBlocks(const void *buffer,size_t start,size_t blockCount) {
	return Ext2RW::writeBlocks(_fs,buffer,start,blockCount);
}

static int open_device(const char *path) {
	int fd = ::open(path,O_RDWR);
	if(fd < 0)
		VTHROWE("Unable to open device '" << path << "'",fd);
	return fd;
}

Ext2FileSystem::Ext2FileSystem(const char *device)
		: fd(open_device(device)), sb(this), bgs(this),
		  inodeCache(this), blockCache(this) {
}

Ext2FileSystem::~Ext2FileSystem() {
	/* write pending changes */
	sync();
	::close(fd);
}

ino_t Ext2FileSystem::find(fs::User *u,fs::OpenFile *dir,const char *name) {
	Ext2CInode *cdir = inodeCache.request(dir->ino,IMODE_READ);
	if(cdir == NULL)
		return -ENOBUFS;

	ino_t ino;
	if(!S_ISDIR(cdir->inode.mode)) {
		ino = -ENOTDIR;
		goto error;
	}
	if((ino = hasPermission(cdir,u,MODE_EXEC)) < 0)
		goto error;

	ino = Ext2Dir::find(this,cdir,name,strlen(name));

error:
	inodeCache.release(cdir);
	return ino;
}

ino_t Ext2FileSystem::open(fs::User *u,const char *path,ssize_t *sympos,ino_t root,uint flags,
		mode_t mode,int fd,fs::OpenFile **file) {
	ino_t ino = Ext2Path::resolve(this,u,path,sympos,root,flags,mode);
	if(ino < 0)
		return ino;

	int err;
	/* check permissions */
	Ext2CInode *cnode = inodeCache.request(ino,IMODE_READ);

	/* opening a symlink, implicitly performs a read */
	if(S_ISLNK(le16tocpu(cnode->inode.mode)))
		flags = O_READ;

	uint imode = 0;
	if(flags & O_READ)
		imode |= MODE_READ;
	if(flags & O_WRITE)
		imode |= MODE_WRITE;
	if(flags & O_EXEC)
		imode |= MODE_EXEC;
	if((err = hasPermission(cnode,u,imode)) < 0) {
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
	*file = new fs::OpenFile(fd,ino);
	return ino;
}

void Ext2FileSystem::close(fs::OpenFile *file) {
	/* decrease references so that we can remove the cached inode and maybe even delete the file */
	Ext2CInode *cnode = inodeCache.request(file->ino,IMODE_READ);
	cnode->refs--;
	inodeCache.release(cnode);
}

int Ext2FileSystem::stat(fs::OpenFile *file,struct stat *info) {
	const Ext2CInode *cnode = inodeCache.request(file->ino,IMODE_READ);
	if(cnode == NULL)
		return -ENOBUFS;

	info->st_atime = le32tocpu(cnode->inode.accesstime);
	info->st_mtime = le32tocpu(cnode->inode.modifytime);
	info->st_ctime = le32tocpu(cnode->inode.createtime);
	info->st_blocks = le32tocpu(cnode->inode.blocks);
	info->st_blksize = blockSize();
	info->st_dev = 0;
	info->st_uid = le16tocpu(cnode->inode.uid);
	info->st_gid = le16tocpu(cnode->inode.gid);
	info->st_ino = cnode->inodeNo;
	info->st_nlink = le16tocpu(cnode->inode.linkCount);
	info->st_mode = le16tocpu(cnode->inode.mode);
	info->st_size = le32tocpu(cnode->inode.size);
	inodeCache.release(cnode);
	return 0;
}

int Ext2FileSystem::chmod(fs::User *u,fs::OpenFile *file,mode_t mode) {
	return Ext2INode::chmod(this,u,file->ino,mode);
}

int Ext2FileSystem::chown(fs::User *u,fs::OpenFile *file,uid_t uid,gid_t gid) {
	return Ext2INode::chown(this,u,file->ino,uid,gid);
}

int Ext2FileSystem::utime(fs::User *u,fs::OpenFile *file,const struct utimbuf *utimes) {
	return Ext2INode::utime(this,u,file->ino,utimes);
}

int Ext2FileSystem::truncate(A_UNUSED fs::User *u,fs::OpenFile *file,off_t length) {
	// TODO implement me!
	if(length > 0)
		return -ENOTSUP;

	Ext2CInode *ino = inodeCache.request(file->ino,IMODE_WRITE);
	if(ino == NULL)
		return -ENOBUFS;
	int res = Ext2File::truncate(this,ino,false);
	inodeCache.release(ino);
	return res;
}

ssize_t Ext2FileSystem::read(fs::OpenFile *file,void *buffer,off_t offset,size_t count) {
	return Ext2File::read(this,file->ino,buffer,offset,count);
}

ssize_t Ext2FileSystem::write(fs::OpenFile *file,const void *buffer,off_t offset,size_t count) {
	return Ext2File::write(this,file->ino,buffer,offset,count);
}

int Ext2FileSystem::link(fs::User *u,fs::OpenFile *dst,fs::OpenFile *dir,const char *name) {
	return linkIno(u,dst->ino,dir,name,false);
}

int Ext2FileSystem::linkIno(fs::User *u,ino_t dst,fs::OpenFile *dir,const char *name,bool isdir) {
	int res;
	Ext2CInode *cdir,*cdst;
	cdir = inodeCache.request(dir->ino,IMODE_WRITE);
	cdst = inodeCache.request(dst,IMODE_WRITE);
	if(cdir == NULL || cdst == NULL)
		res = -ENOBUFS;
	else if(!isdir && S_ISDIR(le16tocpu(cdst->inode.mode)))
		res = -EISDIR;
	else
		res = Ext2Link::create(this,u,cdir,cdst,name);
	inodeCache.release(cdir);
	inodeCache.release(cdst);
	return res;
}

int Ext2FileSystem::doUnlink(fs::User *u,fs::OpenFile *dir,const char *name,bool isdir) {
	int res;
	Ext2CInode *cdir = inodeCache.request(dir->ino,IMODE_WRITE);
	if(cdir == NULL)
		return -ENOBUFS;

	res = Ext2Link::remove(this,u,NULL,cdir,name,isdir);
	inodeCache.release(cdir);
	return res;
}

int Ext2FileSystem::unlink(fs::User *u,fs::OpenFile *dir,const char *name) {
	return doUnlink(u,dir,name,false);
}

int Ext2FileSystem::mkdir(fs::User *u,fs::OpenFile *dir,const char *name,mode_t mode) {
	int res;
	Ext2CInode *cdir = inodeCache.request(dir->ino,IMODE_WRITE);
	if(cdir == NULL)
		return -ENOBUFS;
	res = Ext2Dir::create(this,u,cdir,name,mode);
	inodeCache.release(cdir);
	return res;
}

int Ext2FileSystem::rmdir(fs::User *u,fs::OpenFile *dir,const char *name) {
	int res;
	Ext2CInode *cdir = inodeCache.request(dir->ino,IMODE_WRITE);
	if(cdir == NULL)
		return -ENOBUFS;
	if(!S_ISDIR(le16tocpu(cdir->inode.mode)))
		res = -ENOTDIR;
	else
		res = Ext2Dir::remove(this,u,cdir,name);
	inodeCache.release(cdir);
	return res;
}

int Ext2FileSystem::symlink(fs::User *u,fs::OpenFile *dir,const char *name,const char *target) {
	int res;
	Ext2CInode *cdir = inodeCache.request(dir->ino,IMODE_WRITE);
	if(cdir == NULL)
		return -ENOBUFS;

	if(!S_ISDIR(le16tocpu(cdir->inode.mode)))
		res = -ENOTDIR;
	else {
		ino_t ino;
		res = Ext2File::create(this,u,cdir,name,&ino,S_IFLNK | LNK_DEF_MODE);
		if(res == 0)
			res = Ext2File::write(this,ino,target,0,strlen(target));
	}
	inodeCache.release(cdir);
	return res;
}

int Ext2FileSystem::rename(fs::User *u,fs::OpenFile *oldDir,const char *oldName,fs::OpenFile *newDir,
		const char *newName) {
	ino_t oldFile = find(u,oldDir,oldName);
	if(oldFile < 0)
		return oldFile;
	else {
		/* it might be a directory, too */
		Ext2CInode *oldino = inodeCache.request(oldFile,IMODE_READ);
		if(oldino == NULL)
			return -ENOBUFS;
		bool dir = S_ISDIR(le16tocpu(oldino->inode.mode));
		inodeCache.release(oldino);

		int res = linkIno(u,oldFile,newDir,newName,dir);
		if(res == 0)
			res = doUnlink(u,oldDir,oldName,dir);
		return res;
	}
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

int Ext2FileSystem::hasPermission(Ext2CInode *cnode,fs::User *u,uint perms) {
	mode_t mode = le16tocpu(cnode->inode.mode);
	uid_t uid = le16tocpu(cnode->inode.uid);
	gid_t gid = le16tocpu(cnode->inode.gid);
	return fs::Permissions::canAccess(u,mode,uid,gid,perms);
}

int Ext2FileSystem::canRemove(Ext2CInode *dir,Ext2CInode *file,fs::User *u) {
	mode_t mode = le16tocpu(dir->inode.mode);
	uid_t diruid = le16tocpu(dir->inode.uid);
	uid_t fileuid = le16tocpu(file->inode.uid);
	return fs::Permissions::canRemove(u,mode,diruid,fileuid);
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
