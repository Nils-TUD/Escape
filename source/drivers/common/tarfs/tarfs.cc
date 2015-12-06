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
#include <fs/permissions.h>
#include <sys/common.h>
#include <sys/endian.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <usergroup/group.h>
#include <usergroup/user.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>

#include "file.h"

using namespace esc;

ino_t TarINode::_inode = 1;
static sUser *userList = nullptr;
static sGroup *groupList = nullptr;
static PathTree<TarINode> tree;
static bool changed = false;

struct OpenFile : public Client {
	explicit OpenFile(int f,PathTreeItem<TarINode> *_item = NULL,FILE *_archive = NULL,int _flags = 0)
		: Client(f), flags(_flags), file(_item->getData()), bfile() {
		if(S_ISDIR(_item->getData()->info.st_mode))
			bfile = new DirFile(_item,tree);
		else
			bfile = new RegularFile(_item,_archive,_flags);
		file->reference();
	}
	~OpenFile() {
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
	TarINode *file;
	BlockFile *bfile;
};

class TarFSDevice : public ClientDevice<OpenFile> {
public:
	explicit TarFSDevice(const char *fsDev,FILE *f)
		: ClientDevice<OpenFile>(fsDev,0777,DEV_TYPE_FS,DEV_OPEN | DEV_READ | DEV_WRITE | DEV_CLOSE | DEV_SHFILE),
	 	  _clients(0), _archive(f) {
		init(f);
		set(MSG_FILE_OPEN,std::make_memfun(this,&TarFSDevice::devopen));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&TarFSDevice::devclose),false);
		set(MSG_FS_OPEN,std::make_memfun(this,&TarFSDevice::open));
		set(MSG_FILE_READ,std::make_memfun(this,&TarFSDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&TarFSDevice::write));
		set(MSG_FS_CLOSE,std::make_memfun(this,&TarFSDevice::close),false);
		set(MSG_FS_ISTAT,std::make_memfun(this,&TarFSDevice::istat));
		set(MSG_FS_SYNCFS,std::make_memfun(this,&TarFSDevice::syncfs));
		set(MSG_FS_LINK,std::make_memfun(this,&TarFSDevice::link));
		set(MSG_FS_UNLINK,std::make_memfun(this,&TarFSDevice::unlink));
		set(MSG_FS_RENAME,std::make_memfun(this,&TarFSDevice::rename));
		set(MSG_FS_MKDIR,std::make_memfun(this,&TarFSDevice::mkdir));
		set(MSG_FS_RMDIR,std::make_memfun(this,&TarFSDevice::rmdir));
		set(MSG_FS_CHMOD,std::make_memfun(this,&TarFSDevice::chmod));
		set(MSG_FS_CHOWN,std::make_memfun(this,&TarFSDevice::chown));
		set(MSG_FS_UTIME,std::make_memfun(this,&TarFSDevice::utime));
		set(MSG_FS_TRUNCATE,std::make_memfun(this,&TarFSDevice::truncate));
  	}

	void loop() {
		ulong buf[IPC_DEF_SIZE / sizeof(ulong)];
		while(1) {
			msgid_t mid;
			int fd = getwork(id(),&mid,buf,sizeof(buf),isStopped() ? GW_NOBLOCK : 0);
			if(EXPECT_FALSE(fd < 0)) {
				if(fd != -EINTR) {
					// no requests anymore and we should shutdown?
					if(isStopped())
						break;
					printe("getwork failed");
				}
				continue;
			}

			IPCStream is(fd,buf,sizeof(buf),mid);
			handleMsg(mid,is);
		}
	}

	void devopen(IPCStream &is) {
		_clients++;
		is << 0 << Reply();
	}

	void devclose(IPCStream &is) {
		::close(is.fd());
		if(--_clients == 0)
			stop();
	}

	void open(IPCStream &is) {
		char path[MAX_PATH_LEN];
		FileOpen::Request r(path,sizeof(path));
		is >> r;

		const char *end = NULL;
		PathTreeItem<TarINode> *file = tree.find(r.path.str(),&end);
		if(file == NULL || *end != '\0') {
			if(r.flags & O_CREAT) {
				TarINode *inode = new TarINode(time(NULL),0,r.mode);
				inode->info.st_uid = r.u.uid;
				inode->info.st_gid = r.u.gid;
				tree.insert(r.path.str(),inode);
				file = tree.find(r.path.str(),&end);
			}

			if(file == NULL) {
				is << FileOpen::Response(-ENOENT) << Reply();
				return;
			}
		}
		if(!canReach(&r.u,file)) {
			is << FileOpen::Response(-EPERM) << Reply();
			return;
		}

		add(is.fd(),new OpenFile(is.fd(),file,_archive,r.flags));
		is << FileOpen::Response(is.fd()) << Reply();
	}

	void close(IPCStream &is) {
		ClientDevice::close(is);
	}

	void read(IPCStream &is) {
		OpenFile *file = (*this)[is.fd()];
		FileRead::Request r;
		is >> r;

		DataBuf buf(r.count,file->shm(),r.shmemoff);
		ssize_t res = file->read(buf.data(),r.offset,r.count);

		is << FileRead::Response(res) << Reply();
		if(r.shmemoff == -1) {
			if(res > 0)
				is << ReplyData(buf.data(),res);
		}
	}

	void write(IPCStream &is) {
		OpenFile *file = (*this)[is.fd()];
		FileWrite::Request r;
		is >> r;

		DataBuf buf(r.count,file->shm(),r.shmemoff);
		if(r.shmemoff == -1)
			is >> ReceiveData(buf.data(),r.count);

		ssize_t res = file->write(buf.data(),r.offset,r.count);
		if(res > 0)
			changed = true;
		is << FileWrite::Response(res) << Reply();
	}

	void istat(IPCStream &is) {
		OpenFile *of = (*this)[is.fd()];
		is << 0 << of->file->info << Reply();
	}

	void truncate(IPCStream &is) {
		OpenFile *file = (*this)[is.fd()];
		FSUser u;
		off_t length;
		is >> u.uid >> u.gid >> u.pid >> length;

		int res = file->truncate(length);
		is << res << Reply();
	}

	void syncfs(IPCStream &is) {
		is << 0 << Reply();
	}

	void link(IPCStream &is) {
		// TODO
		is << -ENOTSUP << Reply();
	}

	void unlink(IPCStream &is) {
		FSUser u;
		CStringBuf<MAX_PATH_LEN> path;
		is >> u.uid >> u.gid >> u.pid >> path;

		const char *end = NULL;
		PathTreeItem<TarINode> *file = tree.find(path.str(),&end);
		if(file == NULL || *end != '\0')
			is << -ENOENT << Reply();
		else if(!canReach(&u,file))
			is << -EPERM << Reply();
		else if(S_ISDIR(file->getData()->info.st_mode))
			is << -EISDIR << Reply();
		else if(file->getParent() == file)
			is << -EINVAL << Reply();
		else {
			struct stat *pinfo = &file->getParent()->getData()->info;
			int res = Permissions::canAccess(&u,pinfo->st_mode,pinfo->st_uid,pinfo->st_gid,MODE_WRITE);
			if(res < 0)
				is << res << Reply();
			else {
				TarINode *data = tree.remove(path.str());
				data->deference();
				changed = true;

				is << 0 << Reply();
			}
		}
	}

	void rename(IPCStream &is) {
		char tmp[MAX_PATH_LEN];
		FSUser u;
		CStringBuf<MAX_PATH_LEN> srcPath,dstPath;
		is >> u.uid >> u.gid >> u.pid >> srcPath >> dstPath;

		const char *end = NULL;
		PathTreeItem<TarINode> *srcFile = tree.find(srcPath.str(),&end);
		if(srcFile == NULL || *end != '\0')
			is << -ENOENT << Reply();
		else if(!canReach(&u,srcFile))
			is << -EPERM << Reply();
		else if(srcFile->getParent() == srcFile)
			is << -EINVAL << Reply();
		else {
			struct stat *pinfo = &srcFile->getParent()->getData()->info;
			int res = Permissions::canAccess(&u,pinfo->st_mode,pinfo->st_uid,pinfo->st_gid,MODE_WRITE);
			if(res < 0) {
				is << res << Reply();
				return;
			}

			// check whether the parent exists and whether we can write there
			strncpy(tmp,dstPath.str(),sizeof(tmp));
			char *dstParent = dirname(tmp);

			const char *end = NULL;
			PathTreeItem<TarINode> *parentFile = tree.find(dstParent,&end);
			if(parentFile == NULL || *end != '\0') {
				is << -ENOENT << Reply();
				return;
			}
			else {
				struct stat *pinfo = &parentFile->getData()->info;
				int res = Permissions::canAccess(&u,pinfo->st_mode,pinfo->st_uid,pinfo->st_gid,MODE_WRITE);
				if(!canReach(&u,parentFile) || res < 0) {
					is << -EPERM << Reply();
					return;
				}
			}

			PathTreeItem<TarINode> *dstFile = tree.find(dstPath.str(),&end);
			if(dstFile != NULL && *end == '\0')
				is << -EEXIST << Reply();
			else {
				tree.insert(dstPath.str(),srcFile->getData());
				tree.remove(srcPath.str());
				changed = true;

				is << 0 << Reply();
			}
		}
	}

	void mkdir(IPCStream &is) {
		char tmp[MAX_PATH_LEN];
		FSUser u;
		CStringBuf<MAX_PATH_LEN> path;
		is >> u.uid >> u.gid >> u.pid >> path;

		strncpy(tmp,path.str(),sizeof(tmp));
		char *parent = dirname(tmp);

		const char *end = NULL;
		PathTreeItem<TarINode> *parentFile = tree.find(parent,&end);
		if(parentFile == NULL || *end != '\0') {
			is << -ENOENT << Reply();
			return;
		}
		else if(!canReach(&u,parentFile)) {
			is << -EPERM << Reply();
			return;
		}

		PathTreeItem<TarINode> *file = tree.find(path.str(),&end);
		if(file != NULL && *end == '\0') {
			is << -EEXIST << Reply();
			return;
		}

		struct stat *pinfo = &parentFile->getData()->info;
		int res = Permissions::canAccess(&u,pinfo->st_mode,pinfo->st_uid,pinfo->st_gid,MODE_WRITE);
		if(res < 0)
			is << res << Reply();
		else {
			TarINode *inode = new TarINode(time(NULL),0,DIR_DEF_MODE);
			inode->info.st_uid = u.uid;
			inode->info.st_gid = u.gid;
			tree.insert(path.str(),inode);
			changed = true;

			is << 0 << Reply();
		}
	}

	void rmdir(IPCStream &is) {
		FSUser u;
		CStringBuf<MAX_PATH_LEN> path;
		is >> u.uid >> u.gid >> u.pid >> path;

		const char *end = NULL;
		PathTreeItem<TarINode> *file = tree.find(path.str(),&end);
		if(file == NULL || *end != '\0')
			is << -ENOENT << Reply();
		else if(!canReach(&u,file))
			is << -EPERM << Reply();
		else if(!S_ISDIR(file->getData()->info.st_mode))
			is << -ENOTDIR << Reply();
		else if(tree.begin(file) != tree.end())
			is << -ENOTEMPTY << Reply();
		else if(file->getParent() == file)
			is << -EINVAL << Reply();
		else {
			struct stat *pinfo = &file->getParent()->getData()->info;
			int res = Permissions::canAccess(&u,pinfo->st_mode,pinfo->st_uid,pinfo->st_gid,MODE_WRITE);
			if(res < 0)
				is << res << Reply();
			else {
				TarINode *data = tree.remove(path.str());
				data->deference();
				changed = true;

				is << 0 << Reply();
			}
		}
	}

	void chmod(IPCStream &is) {
		FSUser u;
		CStringBuf<MAX_PATH_LEN> path;
		uint mode;
		is >> u.uid >> u.gid >> u.pid >> path >> mode;

		const char *end = NULL;
		PathTreeItem<TarINode> *file = tree.find(path.str(),&end);
		if(file == NULL || *end != '\0')
			is << -ENOENT << Reply();
		else if(!canReach(&u,file))
			is << -EPERM << Reply();
		else {
			struct stat *info = &file->getData()->info;
			if(!Permissions::canChmod(&u,info->st_uid))
				is << -EPERM << Reply();
			else {
				info->st_mode = (info->st_mode & ~MODE_PERM) | (mode & MODE_PERM);
				changed = true;

				is << 0 << Reply();
			}
		}
	}

	void chown(IPCStream &is) {
		FSUser u;
		CStringBuf<MAX_PATH_LEN> path;
		uid_t uid;
		gid_t gid;
		is >> u.uid >> u.gid >> u.pid >> path >> uid >> gid;

		const char *end = NULL;
		PathTreeItem<TarINode> *file = tree.find(path.str(),&end);
		if(file == NULL || *end != '\0')
			is << -ENOENT << Reply();
		else if(!canReach(&u,file))
			is << -EPERM << Reply();
		else {
			struct stat *info = &file->getData()->info;
			if(!Permissions::canChown(&u,info->st_uid,info->st_gid,uid,gid))
				is << -EPERM << Reply();
			else {
				if(uid != (uid_t)-1)
					info->st_uid = uid;
				if(gid != (gid_t)-1)
					info->st_gid = gid;
				changed = true;

				is << 0 << Reply();
			}
		}
	}

	void utime(IPCStream &is) {
		FSUser u;
		CStringBuf<MAX_PATH_LEN> path;
		struct utimbuf utimes;
		is >> u.uid >> u.gid >> u.pid >> path >> utimes;

		const char *end = NULL;
		PathTreeItem<TarINode> *file = tree.find(path.str(),&end);
		if(file == NULL || *end != '\0')
			is << -ENOENT << Reply();
		else if(!canReach(&u,file))
			is << -EPERM << Reply();
		else {
			struct stat *info = &file->getData()->info;
			if(!Permissions::canUtime(&u,info->st_uid))
				is << -EPERM << Reply();
			else {
				info->st_mtime = utimes.modtime;
				info->st_atime = utimes.actime;
				changed = true;

				is << 0 << Reply();
			}
		}
	}

private:
	void init(FILE *f) {
		// add root directory
		tree.insert("/",new TarINode(time(NULL),0,S_IFDIR | 0777));

		Tar::FileHeader header;
		off_t total = lseek(fileno(f),0,SEEK_END);
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
				sUser *u = user_getByName(userList,header.uname);
				if(u)
					tarfile->info.st_uid = u->uid;
			}
			if(*header.gname) {
				sGroup *g = group_getByName(groupList,header.gname);
				if(g)
					tarfile->info.st_gid = g->gid;
			}

			tree.insert(header.filename,tarfile);

			// to next header
			size_t fsize = strtoul(header.size,NULL,8);
			offset += (fsize + Tar::BLOCK_SIZE * 2 - 1) & ~(Tar::BLOCK_SIZE - 1);
		}
	}

	bool canReach(FSUser *u,PathTreeItem<TarINode> *file) {
		while(file->getParent() != file) {
			struct stat *pinfo = &file->getParent()->getData()->info;
			int res = Permissions::canAccess(u,pinfo->st_mode,pinfo->st_uid,pinfo->st_gid,MODE_EXEC);
			if(res < 0)
				return false;
			file = file->getParent();
		}
		return true;
	}

	uint _clients;
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

	userList = user_parseFromFile(USERS_PATH,nullptr);
	if(!userList)
		printe("Warning: unable to parse users from file");
	groupList = group_parseFromFile(GROUPS_PATH,nullptr);
	if(!groupList)
		printe("Unable to parse groups from file");

	FILE *ar = fopen(argv[2],"r");
	if(ar == NULL)
		error("Unable to open '%s' for reading",argv[2]);
	TarFSDevice dev(argv[1],ar);
	dev.loop();

	// if there was a change, first load all files into memory
	if(changed)
		loadRec(ar,"");
	fclose(ar);
	// now write the entire file again
	if(changed) {
		FILE *f = fopen(argv[2],"w");
		writeBackRec(f,"");
		fclose(f);
	}
	return 0;
}
