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
#include <esc/driver/init.h>
#include <esc/fsinterface.h>
#include <esc/driver.h>
#include <esc/time.h>
#include <esc/dir.h>
#include <ipc/clientdevice.h>
#include <fs/fsdev.h>
#include <fs/infodev.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

using namespace ipc;

FSDevice *FSDevice::_inst;

static void sigTermHndl(A_UNUSED int sig) {
	/* notify init that we're alive and promise to terminate as soon as possible */
	init_iamalive();
	FSDevice::getInstance()->stop();
}

FSDevice::FSDevice(sFileSystem *fs,const char *name,const char *diskDev,const char *fsDev)
	: ClientDevice(fsDev,0777,DEV_TYPE_FS,DEV_OPEN | DEV_READ | DEV_WRITE | DEV_CLOSE | DEV_SHFILE),
	  _fs(fs), _clients(0) {
	set(MSG_DEV_OPEN,std::make_memfun(this,&FSDevice::devopen));
	set(MSG_DEV_CLOSE,std::make_memfun(this,&FSDevice::devclose),false);
	set(MSG_FS_OPEN,std::make_memfun(this,&FSDevice::open));
	set(MSG_DEV_READ,std::make_memfun(this,&FSDevice::read));
	set(MSG_DEV_WRITE,std::make_memfun(this,&FSDevice::write));
	set(MSG_FS_CLOSE,std::make_memfun(this,&FSDevice::close));
	set(MSG_FS_STAT,std::make_memfun(this,&FSDevice::stat));
	set(MSG_FS_ISTAT,std::make_memfun(this,&FSDevice::istat));
	set(MSG_FS_SYNCFS,std::make_memfun(this,&FSDevice::syncfs));
	set(MSG_FS_LINK,std::make_memfun(this,&FSDevice::link));
	set(MSG_FS_UNLINK,std::make_memfun(this,&FSDevice::unlink));
	set(MSG_FS_MKDIR,std::make_memfun(this,&FSDevice::mkdir));
	set(MSG_FS_RMDIR,std::make_memfun(this,&FSDevice::rmdir));
	set(MSG_FS_CHMOD,std::make_memfun(this,&FSDevice::chmod));
	set(MSG_FS_CHOWN,std::make_memfun(this,&FSDevice::chown));

	if(signal(SIG_TERM,sigTermHndl) == SIG_ERR)
		throw std::default_error("Unable to set signal-handler for SIG_TERM");

	int err = 0;
	char *useddev;
	fs->handle = fs->init(diskDev,&useddev,&err);
	if(err < 0)
		VTHROWE("fs->init",err);

	printf("[%s] Mounted '%s'\n",name,useddev);
	fflush(stdout);

	if((err = infodev_start(fsDev,fs)) < 0)
		VTHROWE("infodev_start",err);
	_inst = this;
}

FSDevice::~FSDevice() {
	infodev_shutdown();
	if(_fs->sync)
		_fs->sync(_fs->handle);
}

void FSDevice::loop() {
	char buf[IPC_DEF_SIZE];
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

		IPCStream is(fd,buf,sizeof(buf));
		handleMsg(mid,is);
	}
}

void FSDevice::devopen(IPCStream &is) {
	_clients++;
	is << 0 << Send(MSG_DEV_OPEN_RESP);
}

void FSDevice::devclose(IPCStream &is) {
	::close(is.fd());
	if(--_clients == 0)
		stop();
}

void FSDevice::open(IPCStream &is) {
	uint flags;
	sFSUser u;
	CStringBuf<MAX_PATH_LEN> path;
	is >> flags >> u.uid >> u.gid >> u.pid >> path;

	inode_t no = _fs->resPath(_fs->handle,&u,path.str(),flags);
	if(no >= 0) {
		no = _fs->open(_fs->handle,&u,no,flags);
		if(no >= 0)
			add(is.fd(),new OpenFile(is.fd(),no));
	}
	is << no << Send(MSG_FS_OPEN_RESP);
}

void FSDevice::read(IPCStream &is) {
	OpenFile *file = get(is.fd());
	size_t offset,count;
	ssize_t res,shmemoff;
	is >> offset >> count >> shmemoff;

	void *buffer = NULL;
	if(_fs->read == NULL)
		res = -ENOTSUP;
	else {
		if(shmemoff == -1)
			buffer = malloc(count);
		else
			buffer = (char*)file->shm() + shmemoff;
		if(buffer == NULL)
			res = -ENOMEM;
		else
			res = _fs->read(_fs->handle,file->ino,buffer,offset,count);
	}

	is << res << Send(MSG_DEV_READ_RESP);
	if(buffer && shmemoff == -1) {
		if(res > 0)
			is << SendData(MSG_DEV_READ_RESP,buffer,res);
		free(buffer);
	}
}

void FSDevice::write(IPCStream &is) {
	OpenFile *file = get(is.fd());
	size_t offset,count;
	ssize_t res,shmemoff;
	is >> offset >> count >> shmemoff;

	char *data = file->shm() + shmemoff;
	if(shmemoff == -1) {
		data = new char[count];
		is >> ReceiveData(data,count);
	}

	if(_fs->readonly)
		res = -EROFS;
	else if(_fs->write == NULL)
		res = -ENOTSUP;
	else
		res = _fs->write(_fs->handle,file->ino,data,offset,count);

	is << res << Send(MSG_DEV_WRITE_RESP);
	if(shmemoff == -1)
		delete[] data;
}

void FSDevice::close(IPCStream &is) {
	OpenFile *file = get(is.fd());
	_fs->close(_fs->handle,file->ino);
	ClientDevice::close(is);
}

void FSDevice::stat(IPCStream &is) {
	sFSUser u;
	CStringBuf<MAX_PATH_LEN> path;
	is >> u.uid >> u.gid >> u.pid >> path;

	int res;
	sFileInfo info;
	inode_t no = _fs->resPath(_fs->handle,&u,path.str(),IO_READ);
	if(no < 0)
		res = no;
	else
		res = _fs->stat(_fs->handle,no,&info);

	is << res << info << Send(MSG_FS_STAT_RESP);
}

void FSDevice::istat(IPCStream &is) {
	sFileInfo info;
	int res = _fs->stat(_fs->handle,get(is.fd())->ino,&info);
	is << res << info << Send(MSG_FS_STAT_RESP);
}

void FSDevice::chmod(IPCStream &is) {
	sFSUser u;
	CStringBuf<MAX_PATH_LEN> path;
	uint mode;
	is >> u.uid >> u.gid >> u.pid >> path >> mode;

	int res;
	inode_t ino = _fs->resPath(_fs->handle,&u,path.str(),IO_READ);
	if(ino < 0)
		res = ino;
	else {
		if(_fs->readonly)
			res = -EROFS;
		else if(_fs->chmod == NULL)
			res = -ENOTSUP;
		else
			res = _fs->chmod(_fs->handle,&u,ino,mode);
	}

	is << res << Send(MSG_FS_CHMOD_RESP);
}

void FSDevice::chown(IPCStream &is) {
	sFSUser u;
	CStringBuf<MAX_PATH_LEN> path;
	uid_t uid;
	gid_t gid;
	is >> u.uid >> u.gid >> u.pid >> path >> uid >> gid;

	int res;
	inode_t ino = _fs->resPath(_fs->handle,&u,path.str(),IO_READ);
	if(ino < 0)
		res = ino;
	else {
		if(_fs->readonly)
			res = -EROFS;
		else if(_fs->chown == NULL)
			res = -ENOTSUP;
		else
			res = _fs->chown(_fs->handle,&u,ino,uid,gid);
	}

	is << res << Send(MSG_FS_CHOWN_RESP);
}

static char *splitPath(char *path) {
	/* split path and name */
	size_t len = strlen(path);
	if(path[len - 1] == '/')
		path[len - 1] = '\0';
	char *name = strrchr(path,'/');
	if(name) {
		name = strdup(name + 1);
		dirname(path);
	}
	else {
		name = strdup(path);
		/* we know that path resides in sStrMsg */
		strcpy(path,"/");
	}
	return name;
}

void FSDevice::link(IPCStream &is) {
	sFSUser u;
	CStringBuf<MAX_PATH_LEN> oldPath,newPath;
	is >> u.uid >> u.gid >> u.pid >> oldPath >> newPath;

	int res;
	inode_t dirIno,dstIno;
	dstIno = _fs->resPath(_fs->handle,&u,oldPath.str(),IO_READ);
	if(dstIno < 0)
		res = dstIno;
	else {
		char *name = splitPath(newPath.str());
		res = -ENOMEM;
		if(name) {
			dirIno = _fs->resPath(_fs->handle,&u,newPath.str(),IO_READ);
			if(dirIno < 0)
				res = dirIno;
			else if(_fs->readonly)
				res = -EROFS;
			else if(_fs->chown == NULL)
				res = -ENOTSUP;
			else
				res = _fs->link(_fs->handle,&u,dstIno,dirIno,name);
		}
	}

	is << res << Send(MSG_FS_LINK_RESP);
}

void FSDevice::unlink(IPCStream &is) {
	sFSUser u;
	CStringBuf<MAX_PATH_LEN> path;
	is >> u.uid >> u.gid >> u.pid >> path;

	int res;
	inode_t dirIno = _fs->resPath(_fs->handle,&u,path.str(),IO_READ);
	if(dirIno < 0)
		res = dirIno;
	else {
		char *name = splitPath(path.str());
		res = -ENOMEM;
		if(name) {
			/* get directory */
			dirIno = _fs->resPath(_fs->handle,&u,path.str(),IO_READ);
			vassert(dirIno >= 0,"Subdir found, but parent not!?");
			if(_fs->readonly)
				res = -EROFS;
			else if(_fs->chown == NULL)
				res = -ENOTSUP;
			else
				res = _fs->unlink(_fs->handle,&u,dirIno,name);
		}
	}

	is << res << Send(MSG_FS_UNLINK_RESP);
}

void FSDevice::mkdir(IPCStream &is) {
	sFSUser u;
	CStringBuf<MAX_PATH_LEN> path;
	is >> u.uid >> u.gid >> u.pid >> path;

	int res = -ENOMEM;
	char *name = splitPath(path.str());
	if(name) {
		inode_t dirIno = _fs->resPath(_fs->handle,&u,path.str(),IO_READ);
		if(dirIno < 0)
			res = dirIno;
		else {
			if(_fs->readonly)
				res = -EROFS;
			else if(_fs->mkdir == NULL)
				res = -ENOTSUP;
			else
				res = _fs->mkdir(_fs->handle,&u,dirIno,name);
		}
		free(name);
	}

	is << res << Send(MSG_FS_MKDIR_RESP);
}

void FSDevice::rmdir(IPCStream &is) {
	sFSUser u;
	CStringBuf<MAX_PATH_LEN> path;
	is >> u.uid >> u.gid >> u.pid >> path;

	int res = -ENOMEM;
	char *name = splitPath(path.str());
	if(name) {
		inode_t dirIno = _fs->resPath(_fs->handle,&u,path.str(),IO_READ);
		if(dirIno < 0)
			res = dirIno;
		else {
			if(_fs->readonly)
				res = -EROFS;
			else if(_fs->rmdir == NULL)
				res = -ENOTSUP;
			else
				res = _fs->rmdir(_fs->handle,&u,dirIno,name);
		}
		free(name);
	}

	is << res << Send(MSG_FS_RMDIR_RESP);
}

void FSDevice::syncfs(IPCStream &is) {
	if(_fs->sync)
		_fs->sync(_fs->handle);

	is << 0 << Send(MSG_FS_SYNCFS_RESP);
}
