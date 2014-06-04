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
#include <esc/dir.h>
#include <ipc/proto/socket.h>
#include <ipc/proto/net.h>
#include <ipc/proto/vterm.h>
#include <ipc/clientdevice.h>
#include <dns.h>
#include <stdlib.h>
#include <fstream>

#include "ctrlcon.h"
#include "datacon.h"
#include "dircache.h"
#include "blockfile.h"
#include "dirlist.h"
#include "file.h"

using namespace ipc;

struct OpenFile : public Client {
	explicit OpenFile(int f,CtrlCon *ctrl = NULL,const char *p = NULL,bool isDir = false)
		: Client(f), _path(p),
		  _file(isDir ? (BlockFile*)new DirList(_path,ctrl) : (BlockFile*)new File(_path,ctrl)) {
	}
	virtual ~OpenFile() {
		delete _file;
	}

	const std::string &path() const {
		return _path;
	}

	ssize_t read(void *buffer,size_t offset,size_t count) {
		return _file->read(buffer,offset,count);
	}

private:
	std::string _path;
	BlockFile *_file;
};

class FTPFSDevice : public ClientDevice<OpenFile> {
public:
	explicit FTPFSDevice(const char *fsDev,const char *host,port_t port,const char *user,const char *pw)
		: ClientDevice<OpenFile>(fsDev,0777,DEV_TYPE_FS,DEV_OPEN | DEV_READ | DEV_WRITE | DEV_CLOSE | DEV_SHFILE),
		  _ctrl(new CtrlCon(host,port,user,pw)), _clients() {
		set(MSG_FILE_OPEN,std::make_memfun(this,&FTPFSDevice::devopen));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&FTPFSDevice::devclose),false);
		set(MSG_FS_OPEN,std::make_memfun(this,&FTPFSDevice::open));
		set(MSG_FILE_READ,std::make_memfun(this,&FTPFSDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&FTPFSDevice::write));
		set(MSG_FS_CLOSE,std::make_memfun(this,&FTPFSDevice::close),false);
		set(MSG_FS_STAT,std::make_memfun(this,&FTPFSDevice::stat));
		set(MSG_FS_ISTAT,std::make_memfun(this,&FTPFSDevice::istat));
		set(MSG_FS_SYNCFS,std::make_memfun(this,&FTPFSDevice::syncfs));
		set(MSG_FS_LINK,std::make_memfun(this,&FTPFSDevice::link));
		set(MSG_FS_UNLINK,std::make_memfun(this,&FTPFSDevice::unlink));
		set(MSG_FS_MKDIR,std::make_memfun(this,&FTPFSDevice::mkdir));
		set(MSG_FS_RMDIR,std::make_memfun(this,&FTPFSDevice::rmdir));
		set(MSG_FS_CHMOD,std::make_memfun(this,&FTPFSDevice::chmod));
		set(MSG_FS_CHOWN,std::make_memfun(this,&FTPFSDevice::chown));
  	}
	virtual ~FTPFSDevice() {
		delete _ctrl;
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

		add(is.fd(),new OpenFile(is.fd(),_ctrl,path,isDirectory(path)));
		is << FileOpen::Response(is.fd()) << Reply();
	}

	void read(IPCStream &is) {
		OpenFile *file = (*this)[is.fd()];
		FileRead::Request r;
		is >> r;

		void *buffer = NULL;
		if(r.shmemoff == -1)
			buffer = malloc(r.count);
		else
			buffer = (char*)file->shm() + r.shmemoff;

		ssize_t res;
		if(buffer == NULL)
			res = -ENOMEM;
		else
			res = file->read(buffer,r.offset,r.count);

		is << res << Reply();
		if(buffer && r.shmemoff == -1) {
			if(res > 0)
				is << ReplyData(buffer,res);
			free(buffer);
		}
	}

	void write(IPCStream &is) {
		FileWrite::Request r;
		is >> r;
		if(r.shmemoff == -1)
			is >> ReceiveData(NULL,0);
		is << -ENOTSUP << Reply();
	}

	void close(IPCStream &is) {
		ClientDevice::close(is);
	}

	void stat(IPCStream &is) {
		int uid,gid,pid;
		CStringBuf<MAX_PATH_LEN> path;
		is >> uid >> gid >> pid >> path;

		sFileInfo info;
		int res = getInfo(path.str(),&info);

		is << res << info << Reply();
	}

	void istat(IPCStream &is) {
		sFileInfo info;
		int res = getInfo((*this)[is.fd()]->path().c_str(),&info);
		is << res << info << Reply();
	}

	void syncfs(IPCStream &is) {
		is << 0 << Reply();
	}
	void link(IPCStream &is) {
		is << -ENOTSUP << Reply();
	}
	void unlink(IPCStream &is) {
		is << -ENOTSUP << Reply();
	}
	void mkdir(IPCStream &is) {
		is << -ENOTSUP << Reply();
	}
	void rmdir(IPCStream &is) {
		is << -ENOTSUP << Reply();
	}
	void chmod(IPCStream &is) {
		is << -ENOTSUP << Reply();
	}
	void chown(IPCStream &is) {
		is << -ENOTSUP << Reply();
	}

private:
	bool isDirectory(const char *path) {
		sFileInfo info;
		if(getInfo(path,&info) == 0)
			return S_ISDIR(info.mode);
		return 0;
	}
	int getInfo(const char *path,sFileInfo *info) {
		return DirCache::getInfo(_ctrl,path,info);
	}

	CtrlCon *_ctrl;
	size_t _clients;
};

int main(int argc,char **argv) {
	char path[MAX_PATH_LEN];
	if(argc != 3)
		error("Usage: %s <wait> <url>",argv[0]);

	abspath(path,sizeof(path),argv[2]);
	if(strncmp(path,"/dev/ftp/",9) != 0)
		error("Invalid device: %s",argv[2]);

	char *at = strchr(path + 9,'@');
	if(!at)
		error("Invalid device: %s",argv[2]);
	port_t port = 21;
	char *host = at + 1;
	char *user = path + 9;
	*at = '\0';
	at = strchr(host,':');
	if(at) {
		*at = '\0';
		port = atoi(at + 1);
	}

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

	FTPFSDevice dev(argv[1],host,port,user,pw);
	dev.loop();
	return 0;
}
