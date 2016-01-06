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

#include <esc/ipc/clientdevice.h>
#include <fs/tar/tar.h>
#include <esc/pathtree.h>
#include <fs/filesystem.h>
#include <fs/fsdev.h>
#include <fs/permissions.h>
#include <sys/common.h>
#include <sys/endian.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <usergroup/usergroup.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>

#include "file.h"

using namespace esc;
using namespace fs;

ino_t TarINode::_inode = 1;

static char archiveFile[MAX_PATH_LEN];
static sNamedItem *userList = nullptr;
static sNamedItem *groupList = nullptr;
static PathTree<TarINode> tree;
static bool changed = false;

struct OpenTarFile : public OpenFile {
	explicit OpenTarFile(int f,const char *_path = NULL,PathTreeItem<TarINode> *_item = NULL,
			FILE *_archive = NULL,int _flags = 0)
		: OpenFile(f), flags(_flags), path(_path), file(_item->getData()), bfile() {
		if(S_ISDIR(_item->getData()->info.st_mode))
			bfile = new DirFile(_item,tree);
		else
			bfile = new RegularFile(_item,_archive,_flags);
		file->reference();
	}
	~OpenTarFile() {
		delete bfile;
		file->deference();
	}

	ssize_t read(void *buffer,size_t offset,size_t count) {
		if(~flags & O_READ)
			return -EPERM;

		return bfile->read(buffer,offset,count);
	}

	ssize_t write(const void *buffer,size_t offset,size_t count) {
		if(~flags & O_WRITE)
			return -EPERM;
		if(offset + count < offset)
			return -EINVAL;

		return bfile->write(buffer,offset,count);
	}
	int truncate(off_t length) {
		if(~flags & O_WRITE)
			return -EPERM;
		return bfile->truncate(length);
	}

	int flags;
	std::string path;
	TarINode *file;
	BlockFile *bfile;
};

class TarFileSystem : public FileSystem<OpenTarFile> {
public:
	explicit TarFileSystem(FILE *archive) : FileSystem<OpenTarFile>(), _archive(archive) {
		init(_archive);
	}

	ino_t open(User *u,const char *path,uint flags,mode_t mode,int fd,OpenTarFile **file) override  {
		const char *end = NULL;
		PathTreeItem<TarINode> *tfile = tree.find(path,&end);
		if(file == NULL || *end != '\0') {
			if(flags & O_CREAT) {
				TarINode *inode = new TarINode(time(NULL),0,mode);
				inode->info.st_uid = u->uid;
				inode->info.st_gid = u->gid;
				tree.insert(path,inode);
				tfile = tree.find(path,&end);
			}

			if((~flags & O_CREAT) || file == NULL)
				return -ENOENT;
		}
		if(!canReach(u,tfile))
			return -EPERM;

		*file = new OpenTarFile(fd,path,tfile,_archive,flags);
		return fd;
	}

	void close(OpenTarFile *) override {
	}

	int stat(OpenTarFile *file,struct stat *info) override {
		*info = file->file->info;
		return 0;
	}

	ssize_t read(OpenTarFile *file,void *data,off_t pos,size_t count) override {
		return file->read(data,pos,count);
	}

	ssize_t write(OpenTarFile *file,const void *data,off_t pos,size_t count) override {
		ssize_t res = file->write(data,pos,count);
		if(res > 0)
			changed = true;
		return res;
	}

	int truncate(User *,OpenTarFile *file,off_t length) override {
		return file->truncate(length);
	}

	int link(User *,OpenTarFile *,OpenTarFile *,const char *) override {
		// TODO
		return -ENOTSUP;
	}

	int unlink(User *u,OpenTarFile *dir,const char *name) override {
		char path[MAX_PATH_LEN];
		snprintf(path,sizeof(path),"%s/%s",dir->path.c_str(),name);

		const char *end = NULL;
		PathTreeItem<TarINode> *file = tree.find(path,&end);
		if(file == NULL || *end != '\0')
			return -ENOENT;
		if(S_ISDIR(file->getData()->info.st_mode))
			return -EISDIR;
		if(!S_ISDIR(dir->file->info.st_mode))
			return -ENOTDIR;

		struct stat *pinfo = &dir->file->info;
		int res = Permissions::canAccess(u,pinfo->st_mode,pinfo->st_uid,pinfo->st_gid,MODE_WRITE);
		if(res < 0)
			return res;

		TarINode *data = tree.remove(path);
		data->deference();
		changed = true;
		return 0;
	}

	int mkdir(User *u,OpenTarFile *dir,const char *name,mode_t mode) override {
		char path[MAX_PATH_LEN];
		snprintf(path,sizeof(path),"%s/%s",dir->path.c_str(),name);

		int res;
		const char *end = NULL;
		struct stat *pinfo = &dir->file->info;
		PathTreeItem<TarINode> *file = tree.find(path,&end);
		if(file != NULL && *end == '\0')
			return -EEXIST;
		if(!S_ISDIR(dir->file->info.st_mode))
			return -ENOTDIR;
		if((res = Permissions::canAccess(u,pinfo->st_mode,pinfo->st_uid,pinfo->st_gid,MODE_WRITE)) < 0)
			return res;

		TarINode *inode = new TarINode(time(NULL),0,S_IFDIR | (mode & MODE_PERM));
		inode->info.st_uid = u->uid;
		inode->info.st_gid = u->gid;
		tree.insert(path,inode);
		changed = true;
		return 0;
	}

	int rmdir(User *u,OpenTarFile *dir,const char *name) override {
		char path[MAX_PATH_LEN];
		snprintf(path,sizeof(path),"%s/%s",dir->path.c_str(),name);

		int res;
		const char *end = NULL;
		struct stat *pi = &dir->file->info;
		PathTreeItem<TarINode> *file = tree.find(path,&end);
		if(file == NULL || *end != '\0')
			return -ENOENT;
		if(!S_ISDIR(dir->file->info.st_mode))
			return -ENOTDIR;
		if(tree.begin(file) != tree.end())
			return -ENOTEMPTY;
		if(file->getParent() == file)
			return -EINVAL;
		if((res = Permissions::canAccess(u,pi->st_mode,pi->st_uid,pi->st_gid,MODE_WRITE)) < 0)
			return res;

		TarINode *data = tree.remove(path);
		data->deference();
		changed = true;
		return 0;
	}

	int rename(User *u,OpenTarFile *oldDir,const char *oldName,OpenTarFile *newDir,
			const char *newName) override {
		char oldPath[MAX_PATH_LEN];
		char newPath[MAX_PATH_LEN];
		snprintf(oldPath,sizeof(oldPath),"%s/%s",oldDir->path.c_str(),oldName);
		snprintf(newPath,sizeof(newPath),"%s/%s",newDir->path.c_str(),newName);

		int res;
		const char *end = NULL;
		struct stat *opi = &oldDir->file->info;
		struct stat *npi = &newDir->file->info;
		PathTreeItem<TarINode> *srcFile = tree.find(oldPath,&end);
		if(srcFile == NULL || *end != '\0')
			return -ENOENT;
		if(!S_ISDIR(oldDir->file->info.st_mode))
			return -ENOTDIR;
		if((res = Permissions::canAccess(u,opi->st_mode,opi->st_uid,opi->st_gid,MODE_EXEC)) < 0)
			return res;
		if(srcFile->getParent() == srcFile)
			return -EINVAL;
		if(!S_ISDIR(newDir->file->info.st_mode))
			return -ENOTDIR;
		if((res = Permissions::canAccess(u,npi->st_mode,npi->st_uid,npi->st_gid,MODE_EXEC | MODE_WRITE)) < 0)
			return res;

		PathTreeItem<TarINode> *dstFile = tree.find(newPath,&end);
		if(dstFile != NULL && *end == '\0')
			return -ENOENT;

		tree.insert(newPath,srcFile->getData());
		tree.remove(oldPath);
		changed = true;
		return 0;
	}

	int chmod(User *u,OpenTarFile *file,mode_t mode) override {
		struct stat *info = &file->file->info;
		if(!Permissions::canChmod(u,info->st_uid))
			return -EPERM;

		info->st_mode = (info->st_mode & ~MODE_PERM) | (mode & MODE_PERM);
		changed = true;
		return 0;
	}

	int chown(User *u,OpenTarFile *file,uid_t uid,gid_t gid) override {
		struct stat *info = &file->file->info;
		if(!Permissions::canChown(u,info->st_uid,info->st_gid,uid,gid))
			return -EPERM;

		if(uid != (uid_t)-1)
			info->st_uid = uid;
		if(gid != (gid_t)-1)
			info->st_gid = gid;
		changed = true;
		return 0;
	}

	int utime(User *u,OpenTarFile *file,const struct utimbuf *utimes) override {
		struct stat *info = &file->file->info;
		if(!Permissions::canUtime(u,info->st_uid))
			return -EPERM;

		info->st_mtime = utimes->modtime;
		info->st_atime = utimes->actime;
		changed = true;
		return 0;
	}

	void print(FILE *f) override {
		fprintf(f,"file : %s\n",archiveFile);
		fprintf(f,"dirty: %s\n",changed ? "yes" : "no");
	}

private:
	void init(FILE *f) {
		// add root directory
		tree.insert("/",new TarINode(time(NULL),0,S_IFDIR | 0777));

		Tar::FileHeader header;
		off_t total = filesize(fileno(f));
		off_t offset = 0;
		while(offset < total) {
			Tar::readHeader(f,offset,&header);
			if(header.filename[0] == '\0')
				break;

			TarINode *tarfile = new TarINode(
				strtoul(header.mtime,NULL,8),
				strtoul(header.size,NULL,8),
				strtoul(header.mode,NULL,8)
			);
			tarfile->offset = offset + Tar::BLOCK_SIZE;
			switch(header.type) {
				case Tar::T_REGULAR:
					tarfile->info.st_mode |= S_IFREG;
					break;
				case Tar::T_DIR:
					tarfile->info.st_mode |= S_IFDIR;
					break;
				case Tar::T_BLKDEV:
					tarfile->info.st_mode |= S_IFBLK;
					break;
				case Tar::T_CHARDEV:
					tarfile->info.st_mode |= S_IFCHR;
					break;
				case Tar::T_SYMLINK:
					tarfile->info.st_mode |= S_IFLNK;
					break;
			}
			tarfile->info.st_uid = strtoul(header.uid,NULL,8);
			tarfile->info.st_gid = strtoul(header.gid,NULL,8);

			if(*header.uname) {
				sNamedItem *u = usergroup_getByName(userList,header.uname);
				if(u)
					tarfile->info.st_uid = u->id;
			}
			if(*header.gname) {
				sNamedItem *g = usergroup_getByName(groupList,header.gname);
				if(g)
					tarfile->info.st_gid = g->id;
			}

			tree.insert(header.filename,tarfile);

			// to next header
			size_t fsize = strtoul(header.size,NULL,8);
			offset += (fsize + Tar::BLOCK_SIZE * 2 - 1) & ~(Tar::BLOCK_SIZE - 1);
		}
	}

	bool canReach(User *u,PathTreeItem<TarINode> *file) {
		while(file->getParent() != file) {
			struct stat *pinfo = &file->getParent()->getData()->info;
			int res = Permissions::canAccess(u,pinfo->st_mode,pinfo->st_uid,pinfo->st_gid,MODE_EXEC);
			if(res < 0)
				return false;
			file = file->getParent();
		}
		return true;
	}

	FILE *_archive;
};

static char buffer[Tar::BLOCK_SIZE];
static off_t offset = 0;

static void loadRec(FILE *f,const std::string &path) {
	PathTreeItem<TarINode> *dir = tree.find(path.c_str());
	assert(dir != NULL);
	for(auto it = tree.begin(dir); it != tree.end(); ++it) {
		if(S_ISDIR(it->getData()->info.st_mode))
			loadRec(f,path + "/" + it->getName());
		else if(it->getData()->data == NULL && it->getData()->info.st_size > 0) {
			it->getData()->data = new char[it->getData()->info.st_size];
			fseek(f,it->getData()->offset,SEEK_SET);
			fread(it->getData()->data,it->getData()->info.st_size,1,f);
		}
	}
}

static void writeBackRec(FILE *f,const std::string &path) {
	PathTreeItem<TarINode> *dir = tree.find(path.c_str());
	assert(dir != NULL);
	for(auto it = tree.begin(dir); it != tree.end(); ++it) {
		// don't put absolute paths in an archive
		std::string filepath = path;
		if(!path.empty())
			filepath += std::string("/") + it->getName();
		else
			filepath += it->getName();

		Tar::writeHeader(f,offset,buffer,filepath.c_str(),it->getData()->info,userList,groupList);
		offset += Tar::BLOCK_SIZE;

		if(S_ISDIR(it->getData()->info.st_mode))
			writeBackRec(f,filepath);
		else {
			for(off_t total = 0; total < it->getData()->info.st_size; ) {
				size_t amount = std::min<size_t>(Tar::BLOCK_SIZE,it->getData()->info.st_size - total);
				if(amount < Tar::BLOCK_SIZE) {
					memcpy(buffer,it->getData()->data + total,amount);
					memset(buffer + amount,0,Tar::BLOCK_SIZE - amount);
					Tar::writeBlock(f,offset,buffer);
				}
				else
					Tar::writeBlock(f,offset,it->getData()->data + total);
				offset += Tar::BLOCK_SIZE;
				total += amount;
			}
		}
	}
}

int main(int argc,char **argv) {
	if(argc != 3)
		error("Usage: %s <wait> <tar-file>",argv[0]);

	userList = usergroup_parse(USERS_PATH,nullptr);
	if(!userList)
		printe("Warning: unable to get user list");
	groupList = usergroup_parse(GROUPS_PATH,nullptr);
	if(!groupList)
		printe("Unable to get group list");

	abspath(archiveFile,sizeof(archiveFile),argv[2]);

	FILE *ar = fopen(archiveFile,"r");
	if(ar == NULL)
		error("Unable to open '%s' for reading",archiveFile);

	{
		TarFileSystem fs(ar);
		FSDevice<OpenTarFile> dev(&fs,argv[1]);
		dev.loop();
	}

	// if there was a change, first load all files into memory
	if(changed)
		loadRec(ar,"");
	fclose(ar);
	// now write the entire file again
	if(changed) {
		FILE *f = fopen(archiveFile,"w");
		if(f) {
			writeBackRec(f,"");
			fclose(f);
		}
		else
			printe("Unable to open '%s' for writing",archiveFile);
	}
	return 0;
}
