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
#include <esc/proto/fs.h>
#include <esc/proto/net.h>
#include <esc/proto/socket.h>
#include <esc/proto/vterm.h>
#include <fs/filesystem.h>
#include <fs/fsdev.h>
#include <esc/dns.h>
#include <sys/common.h>
#include <dirent.h>
#include <stdlib.h>

#include "blockfile.h"
#include "ctrlcon.h"
#include "datacon.h"
#include "dircache.h"
#include "dirlist.h"
#include "file.h"

using namespace esc;
using namespace fs;

static const char *host = NULL;
static const char *user = NULL;
static const char *dir = "/";
static port_t port = 21;

struct OpenFTPFile : public OpenFile {
	explicit OpenFTPFile(int f,const CtrlConRef &_ctrlRef = CtrlConRef(),const char *_path = NULL,
			int _flags = 0)
		: OpenFile(f), flags(_flags), path(_path), ctrlRef(_ctrlRef), file(getFile(_ctrlRef,path)) {
	}
	virtual ~OpenFTPFile() {
		delete file;
	}

	ssize_t read(void *buffer,size_t offset,size_t count) {
		if(~flags & O_READ)
			return -EPERM;
		return file->read(buffer,offset,count);
	}

	ssize_t write(const void *buffer,size_t offset,size_t count) {
		if(~flags & O_WRITE)
			return -EPERM;
		file->write(buffer,offset,count);
		return count;
	}

	int sharemem(void *mem,size_t size) {
		return file->sharemem(mem,size);
	}

	static BlockFile *getFile(const CtrlConRef &ctrlRef,const std::string &path) {
		struct stat info;
		if(DirCache::getInfo(ctrlRef,path.c_str(),&info) < 0)
			info.st_size = 0;
		else if(S_ISDIR(info.st_mode))
			return new DirList(path,ctrlRef);
		return new File(path,info.st_size,ctrlRef);
	}

	int flags;
	std::string path;
	CtrlConRef ctrlRef;
	BlockFile *file;
};

class FTPFileSystem : public FileSystem<OpenFTPFile> {
public:
	explicit FTPFileSystem(CtrlConRef &ctrl) : FileSystem<OpenFTPFile>(), _ctrlRef(ctrl) {
	}

	ino_t open(User *,const char *path,uint flags,mode_t,int fd,OpenFTPFile **file) override {
		*file = new OpenFTPFile(fd,_ctrlRef,path,flags);
		return fd;
	}

	void close(OpenFTPFile *file) override {
		/* if it was opened for writing, check if the file exists in the cache */
		if(file->flags & O_WRITE) {
			/* if not found, remove the directory from cache so that we load it again */
			if(DirCache::getList(file->ctrlRef,file->path.c_str(),false) == NULL)
				DirCache::removeDirOf(file->path.c_str());
		}
	}

	int stat(OpenFTPFile *file,struct stat *info) override {
		return DirCache::getInfo(file->ctrlRef,file->path.c_str(),info);
	}

	ssize_t read(OpenFTPFile *file,void *data,off_t pos,size_t count) override {
		return file->read(data,pos,count);
	}

	ssize_t write(OpenFTPFile *file,const void *data,off_t pos,size_t count) override {
		return file->write(data,pos,count);
	}

	int unlink(User *,OpenFTPFile *dir,const char *name) override {
		dirCmd(dir,name,CtrlCon::CMD_DELE);
		return 0;
	}

	int mkdir(User *,OpenFTPFile *dir,const char *name,mode_t) override {
		dirCmd(dir,name,CtrlCon::CMD_MKD);
		return 0;
	}

	int rmdir(User *,OpenFTPFile *dir,const char *name) override {
		dirCmd(dir,name,CtrlCon::CMD_RMD);
		return 0;
	}

	int rename(User *,OpenFTPFile *oldDir,const char *oldName,OpenFTPFile *newDir,
			const char *newName) override {
		char oldPath[MAX_PATH_LEN];
		char newPath[MAX_PATH_LEN];

		snprintf(oldPath,sizeof(oldPath),"%s/%s",oldDir->path.c_str(),oldName);
		snprintf(oldPath,sizeof(newPath),"%s/%s",newDir->path.c_str(),newName);

		CtrlConRef ref(_ctrlRef);
		CtrlCon *ctrl = ref.request();
		ctrl->execute(CtrlCon::CMD_RNFR,oldPath);
		ctrl->execute(CtrlCon::CMD_RNTO,newPath);
		DirCache::removeDir(oldDir->path.c_str());
		DirCache::removeDir(newDir->path.c_str());
		return 0;
	}

	void print(FILE *f) override {
		fprintf(f,"host: %s\n",host);
		fprintf(f,"port: %d\n",port);
		fprintf(f,"user: %s\n",user);
		fprintf(f,"dir : %s\n",dir);
	}

private:
	void dirCmd(OpenFTPFile *dir,const char *name,CtrlCon::Cmd cmd) {
		char path[MAX_PATH_LEN];

		snprintf(path,sizeof(path),"%s/%s",dir->path.c_str(),name);

		CtrlConRef ref(_ctrlRef);
		CtrlCon *ctrl = ref.request();
		ctrl->execute(cmd,path);
		DirCache::removeDir(dir->path.c_str());
	}

	CtrlConRef &_ctrlRef;
};

class FTPFSDevice : public FSDevice<OpenFTPFile> {
public:
	explicit FTPFSDevice(FTPFileSystem *fs,const char *fsDev)
		: FSDevice<OpenFTPFile>(fs,fsDev) {
		set(MSG_DEV_SHFILE,std::make_memfun(this,&FTPFSDevice::shfile));
	}

	void shfile(IPCStream &is) {
		char path[MAX_PATH_LEN];
		FileShFile::Request r(path,sizeof(path));
		is >> r;

		OpenFTPFile *file = (*this)[is.fd()];

		int res = joinshm(file,path,r.size,0);
		if(res == 0)
			res = file->sharemem(file->sharedmem()->addr,r.size);
		is << FileShFile::Response(res) << Reply();
	}
};

int main(int argc,char **argv) {
	char path[MAX_PATH_LEN];
	if(argc != 3)
		error("Usage: %s <wait> <url>",argv[0]);

	abspath(path,sizeof(path),argv[2]);
	if(strncmp(path,"/dev/ftp/",9) != 0)
		error("Invalid device: %s",argv[2]);

	// use the last one. the username might be an email-address
	char *pos = strrchr(path + 9,'@');
	if(!pos) {
		host = path + 9;
		user = const_cast<char*>("anonymous");
	}
	else {
		host = pos + 1;
		user = path + 9;
		*pos = '\0';
	}

	// get the port
	port_t port = 21;
	pos = strchr(host,':');
	if(pos) {
		*pos++ = '\0';
		port = atoi(pos);
	}
	else
		pos = const_cast<char*>(host);

	// get the directory
	pos = strchr(pos,'/');
	if(pos) {
		*pos = '\0';
		dir = pos + 1;
	}

	// let the user type in the password, if necessary
	char pw[64] = "anonymous@example.com";
	if(strcmp(user,"anonymous") != 0) {
		VTerm vterm(STDOUT_FILENO);
		printf("Password: ");
		vterm.setFlag(VTerm::FL_ECHO,false);
		fgetl(pw,sizeof(pw),stdin);
		clearerr(stdin);
		vterm.setFlag(VTerm::FL_ECHO,true);
		putchar('\n');
		fflush(stdout);
	}

	CtrlConRef con(new CtrlCon(host,port,user,pw,dir));
	FTPFileSystem fs(con);
	FTPFSDevice dev(&fs,argv[1]);
	dev.loop();
	return 0;
}
