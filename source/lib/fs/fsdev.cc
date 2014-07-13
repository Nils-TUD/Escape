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

#include <sys/common.h>
#include <sys/driver.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <esc/ipc/clientdevice.h>
#include <esc/proto/init.h>
#include <fs/fsdev.h>
#include <fs/infodev.h>
#include <signal.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

using namespace esc;

FSDevice *FSDevice::_inst;

static void sigTermHndl(A_UNUSED int sig) {
	/* notify init that we're alive and promise to terminate as soon as possible */
	esc::Init init("/dev/init");
	init.iamalive();
	FSDevice::getInstance()->stop();
}

FSDevice::FSDevice(FileSystem *fs,const char *fsDev)
	: ClientDevice(fsDev,0777,DEV_TYPE_FS,DEV_OPEN | DEV_READ | DEV_WRITE | DEV_CLOSE | DEV_SHFILE),
	  _fs(fs), _info(fsDev,fs), _clients(0) {
	set(MSG_FILE_OPEN,std::make_memfun(this,&FSDevice::devopen));
	set(MSG_FILE_CLOSE,std::make_memfun(this,&FSDevice::devclose),false);
	set(MSG_FS_OPEN,std::make_memfun(this,&FSDevice::open));
	set(MSG_FILE_READ,std::make_memfun(this,&FSDevice::read));
	set(MSG_FILE_WRITE,std::make_memfun(this,&FSDevice::write));
	set(MSG_FS_CLOSE,std::make_memfun(this,&FSDevice::close),false);
	set(MSG_FS_STAT,std::make_memfun(this,&FSDevice::stat));
	set(MSG_FS_ISTAT,std::make_memfun(this,&FSDevice::istat));
	set(MSG_FS_SYNCFS,std::make_memfun(this,&FSDevice::syncfs));
	set(MSG_FS_LINK,std::make_memfun(this,&FSDevice::link));
	set(MSG_FS_UNLINK,std::make_memfun(this,&FSDevice::unlink));
	set(MSG_FS_RENAME,std::make_memfun(this,&FSDevice::rename));
	set(MSG_FS_MKDIR,std::make_memfun(this,&FSDevice::mkdir));
	set(MSG_FS_RMDIR,std::make_memfun(this,&FSDevice::rmdir));
	set(MSG_FS_CHMOD,std::make_memfun(this,&FSDevice::chmod));
	set(MSG_FS_CHOWN,std::make_memfun(this,&FSDevice::chown));

	if(signal(SIGTERM,sigTermHndl) == SIG_ERR)
		throw std::default_error("Unable to set signal-handler for SIGTERM");
	_inst = this;
}

FSDevice::~FSDevice() {
	_fs->sync();
}

void FSDevice::loop() {
	ulong buf[IPC_DEF_SIZE / sizeof(ulong)];
	while(1) {
		msgid_t mid;
		int fd = getwork(id(),&mid,buf,sizeof(buf),isStopped() ? GW_NOBLOCK : 0);
		if(EXPECT_FALSE(fd < 0)) {
			if(fd != -EINTR) {
				/* no requests anymore and we should shutdown? */
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

void FSDevice::devopen(IPCStream &is) {
	_clients++;
	is << 0 << Reply();
}

void FSDevice::devclose(IPCStream &is) {
	::close(is.fd());
	if(--_clients == 0)
		stop();
}

void FSDevice::open(IPCStream &is) {
	char path[MAX_PATH_LEN];
	FSUser u;
	FileOpen::Request r(path,sizeof(path));
	is >> r;

	u.gid = r.gid;
	u.uid = r.uid;
	u.pid = r.pid;
	ino_t no = _fs->resolve(&u,path,r.flags);
	if(no >= 0) {
		no = _fs->open(&u,no,r.flags);
		if(no >= 0)
			add(is.fd(),new OpenFile(is.fd(),no));
	}
	is << no << Reply();
}

void FSDevice::read(IPCStream &is) {
	OpenFile *file = (*this)[is.fd()];
	FileRead::Request r;
	is >> r;

	DataBuf buf(r.count,file->shm(),r.shmemoff);
	ssize_t res = _fs->read(file->ino,buf.data(),r.offset,r.count);

	is << res << Reply();
	if(r.shmemoff == -1 && res > 0)
		is << ReplyData(buf.data(),res);
}

void FSDevice::write(IPCStream &is) {
	OpenFile *file = (*this)[is.fd()];
	FileWrite::Request r;
	is >> r;

	DataBuf buf(r.count,file->shm(),r.shmemoff);
	if(r.shmemoff == -1)
		is >> ReceiveData(buf.data(),r.count);

	ssize_t res = _fs->write(file->ino,buf.data(),r.offset,r.count);
	is << res << Reply();
}

void FSDevice::close(IPCStream &is) {
	OpenFile *file = (*this)[is.fd()];
	_fs->close(file->ino);
	ClientDevice::close(is);
}

void FSDevice::stat(IPCStream &is) {
	FSUser u;
	CStringBuf<MAX_PATH_LEN> path;
	is >> u.uid >> u.gid >> u.pid >> path;

	int res;
	struct stat info;
	ino_t no = _fs->resolve(&u,path.str(),O_RDONLY);
	if(no < 0)
		res = no;
	else
		res = _fs->stat(no,&info);

	is << res << info << Reply();
}

void FSDevice::istat(IPCStream &is) {
	struct stat info;
	int res = _fs->stat((*this)[is.fd()]->ino,&info);
	is << res << info << Reply();
}

void FSDevice::chmod(IPCStream &is) {
	FSUser u;
	CStringBuf<MAX_PATH_LEN> path;
	uint mode;
	is >> u.uid >> u.gid >> u.pid >> path >> mode;

	int res;
	ino_t ino = _fs->resolve(&u,path.str(),O_RDONLY);
	if(ino < 0)
		res = ino;
	else
		res = _fs->chmod(&u,ino,mode);

	is << res << Reply();
}

void FSDevice::chown(IPCStream &is) {
	FSUser u;
	CStringBuf<MAX_PATH_LEN> path;
	uid_t uid;
	gid_t gid;
	is >> u.uid >> u.gid >> u.pid >> path >> uid >> gid;

	int res;
	ino_t ino = _fs->resolve(&u,path.str(),O_RDONLY);
	if(ino < 0)
		res = ino;
	else
		res = _fs->chown(&u,ino,uid,gid);

	is << res << Reply();
}

static const char *splitPath(char *path) {
	static char tmppath[MAX_PATH_LEN];
	/* split path and name */
	size_t len = strlen(path);
	if(path[len - 1] == '/')
		path[--len] = '\0';
	char *name = path + len - 1;
	while(len > 0 && *name != '/') {
		name--;
		len--;
	}
	if(len > 0) {
		strnzcpy(tmppath,name + 1,sizeof(tmppath));
		dirname(path);
	}
	else {
		strnzcpy(tmppath,path,sizeof(tmppath));
		/* we know that path is MAX_PATH_LEN long */
		path[0] = '/';
		path[1] = '\0';
	}
	return tmppath;
}

const char *FSDevice::resolveDir(FSUser *u,char *path,ino_t *ino) {
	const char *name = splitPath(path);
	if(name)
		*ino = _fs->resolve(u,path,O_RDONLY);
	else
		*ino = -ENOMEM;
	return name;
}

void FSDevice::link(IPCStream &is) {
	FSUser u;
	CStringBuf<MAX_PATH_LEN> oldPath,newPath;
	is >> u.uid >> u.gid >> u.pid >> oldPath >> newPath;

	int res;
	ino_t dirIno,dstIno;
	dstIno = _fs->resolve(&u,oldPath.str(),O_RDONLY);
	if(dstIno < 0)
		res = dstIno;
	else {
		const char *name = resolveDir(&u,newPath.str(),&dirIno);
		if(dirIno < 0)
			res = dirIno;
		else
			res = _fs->link(&u,dstIno,dirIno,name);
	}

	is << res << Reply();
}

void FSDevice::unlink(IPCStream &is) {
	FSUser u;
	CStringBuf<MAX_PATH_LEN> path;
	is >> u.uid >> u.gid >> u.pid >> path;

	int res;
	ino_t dirIno = _fs->resolve(&u,path.str(),O_RDONLY);
	if(dirIno < 0)
		res = dirIno;
	else {
		const char *name = resolveDir(&u,path.str(),&dirIno);
		vassert(dirIno >= 0,"Subdir found, but parent not!?");
		res = _fs->unlink(&u,dirIno,name);
	}

	is << res << Reply();
}

void FSDevice::rename(IPCStream &is) {
	FSUser u;
	CStringBuf<MAX_PATH_LEN> oldPath,newPath;
	is >> u.uid >> u.gid >> u.pid >> oldPath >> newPath;

	int res;
	ino_t dirIno,dstIno;
	dstIno = _fs->resolve(&u,oldPath.str(),O_RDONLY);
	if(dstIno < 0)
		res = dstIno;
	else {
		const char *name = resolveDir(&u,newPath.str(),&dirIno);
		if(dirIno < 0)
			res = dirIno;
		else {
			res = _fs->link(&u,dstIno,dirIno,name);
			if(res == 0) {
				name = resolveDir(&u,oldPath.str(),&dirIno);
				if(dirIno < 0)
					res = dirIno;
				else
					res = _fs->unlink(&u,dirIno,name);
			}
		}
	}

	is << res << Reply();
}

void FSDevice::mkdir(IPCStream &is) {
	FSUser u;
	CStringBuf<MAX_PATH_LEN> path;
	is >> u.uid >> u.gid >> u.pid >> path;

	int res;
	ino_t dirIno;
	const char *name = resolveDir(&u,path.str(),&dirIno);
	if(dirIno < 0)
		res = dirIno;
	else
		res = _fs->mkdir(&u,dirIno,name);

	is << res << Reply();
}

void FSDevice::rmdir(IPCStream &is) {
	FSUser u;
	CStringBuf<MAX_PATH_LEN> path;
	is >> u.uid >> u.gid >> u.pid >> path;

	int res = -ENOMEM;
	ino_t dirIno;
	const char *name = resolveDir(&u,path.str(),&dirIno);
	if(dirIno < 0)
		res = dirIno;
	else
		res = _fs->rmdir(&u,dirIno,name);

	is << res << Reply();
}

void FSDevice::syncfs(IPCStream &is) {
	_fs->sync();

	is << 0 << Reply();
}
