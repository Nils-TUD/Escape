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
#include <esc/proto/init.h>
#include <fs/fsdev.h>
#include <fs/infodev.h>
#include <sys/common.h>
#include <sys/driver.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <assert.h>
#include <dirent.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

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
	set(MSG_FS_ISTAT,std::make_memfun(this,&FSDevice::istat));
	set(MSG_FS_SYNCFS,std::make_memfun(this,&FSDevice::syncfs));
	set(MSG_FS_LINK,std::make_memfun(this,&FSDevice::link));
	set(MSG_FS_UNLINK,std::make_memfun(this,&FSDevice::unlink));
	set(MSG_FS_RENAME,std::make_memfun(this,&FSDevice::rename));
	set(MSG_FS_MKDIR,std::make_memfun(this,&FSDevice::mkdir));
	set(MSG_FS_RMDIR,std::make_memfun(this,&FSDevice::rmdir));
	set(MSG_FS_CHMOD,std::make_memfun(this,&FSDevice::chmod));
	set(MSG_FS_CHOWN,std::make_memfun(this,&FSDevice::chown));
	set(MSG_FS_UTIME,std::make_memfun(this,&FSDevice::utime));
	set(MSG_FS_TRUNCATE,std::make_memfun(this,&FSDevice::truncate));

	if(signal(SIGTERM,sigTermHndl) == SIG_ERR)
		throw esc::default_error("Unable to set signal-handler for SIGTERM");
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
	FileOpen::Request r(path,sizeof(path));
	is >> r;

	ino_t no = _fs->resolve(&r.u,path,r.flags,S_IFREG | (r.mode & MODE_PERM));
	if(no >= 0) {
		no = _fs->open(&r.u,no,r.flags);
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

void FSDevice::istat(IPCStream &is) {
	struct stat info;
	int res = _fs->stat((*this)[is.fd()]->ino,&info);
	is << res << info << Reply();
}

void FSDevice::chmod(IPCStream &is) {
	OpenFile *file = (*this)[is.fd()];
	FSUser u;
	uint mode;
	is >> u.uid >> u.gid >> u.pid >> mode;

	int res = _fs->chmod(&u,file->ino,mode);
	is << res << Reply();
}

void FSDevice::chown(IPCStream &is) {
	OpenFile *file = (*this)[is.fd()];
	FSUser u;
	uid_t uid;
	gid_t gid;
	is >> u.uid >> u.gid >> u.pid >> uid >> gid;

	int res = _fs->chown(&u,file->ino,uid,gid);
	is << res << Reply();
}

void FSDevice::utime(IPCStream &is) {
	OpenFile *file = (*this)[is.fd()];
	FSUser u;
	struct utimbuf utimes;
	is >> u.uid >> u.gid >> u.pid >> utimes;

	int res = _fs->utime(&u,file->ino,&utimes);
	is << res << Reply();
}

void FSDevice::truncate(IPCStream &is) {
	FSUser u;
	off_t length;
	is >> u.uid >> u.gid >> u.pid >> length;

	int res = _fs->truncate(&u,(*this)[is.fd()]->ino,length);
	is << res << Reply();
}

void FSDevice::link(IPCStream &is) {
	FSUser u;
	int dirFd;
	CStringBuf<MAX_PATH_LEN> name;
	is >> u.uid >> u.gid >> u.pid >> dirFd >> name;

	OpenFile *targetFile = (*this)[is.fd()];
	OpenFile *dirFile = (*this)[dirFd];

	int res = _fs->link(&u,targetFile->ino,dirFile->ino,name.str());
	is << res << Reply();
}

void FSDevice::unlink(IPCStream &is) {
	OpenFile *dir = (*this)[is.fd()];
	FSUser u;
	CStringBuf<MAX_PATH_LEN> name;
	is >> u.uid >> u.gid >> u.pid >> name;

	int res = _fs->unlink(&u,dir->ino,name.str());
	is << res << Reply();
}

void FSDevice::rename(IPCStream &is) {
	FSUser u;
	int newDirFd;
	CStringBuf<MAX_PATH_LEN> oldName,newName;
	is >> u.uid >> u.gid >> u.pid >> oldName >> newDirFd >> newName;

	OpenFile *oldDir = (*this)[is.fd()];
	OpenFile *newDir = (*this)[newDirFd];

	ino_t oldFile = _fs->find(&u,oldDir->ino,oldName.str());
	if(oldFile < 0)
		is << oldFile << Reply();
	else {
		int res = _fs->link(&u,oldFile,newDir->ino,newName.str());
		if(res == 0)
			res = _fs->unlink(&u,oldDir->ino,oldName.str());
		is << res << Reply();
	}
}

void FSDevice::mkdir(IPCStream &is) {
	OpenFile *file = (*this)[is.fd()];
	FSUser u;
	mode_t mode;
	CStringBuf<MAX_PATH_LEN> name;
	is >> u.uid >> u.gid >> u.pid >> name >> mode;

	int res = _fs->mkdir(&u,file->ino,name.str(),mode);
	is << res << Reply();
}

void FSDevice::rmdir(IPCStream &is) {
	OpenFile *file = (*this)[is.fd()];
	FSUser u;
	CStringBuf<MAX_PATH_LEN> name;
	is >> u.uid >> u.gid >> u.pid >> name;

	int res = _fs->rmdir(&u,file->ino,name.str());
	is << res << Reply();
}

void FSDevice::syncfs(IPCStream &is) {
	_fs->sync();

	is << 0 << Reply();
}
