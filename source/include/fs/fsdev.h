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

#pragma once

#include <esc/ipc/clientdevice.h>
#include <esc/proto/fs.h>
#include <esc/proto/init.h>
#include <fs/common.h>
#include <sys/common.h>
#include <sys/stat.h>
#include <stdio.h>

namespace fs {

template<class F>
class FileSystem;

struct OpenFile : public esc::Client {
	explicit OpenFile(int fd) : Client(fd), ino() {
	}
	explicit OpenFile(int fd,ino_t _ino) : Client(fd), ino(_ino) {
	}

	ino_t ino;
};

template<class F>
class FSDevice : public esc::ClientDevice<F> {
public:
	explicit FSDevice(FileSystem<F> *fs,const char *fsDev)
		: esc::ClientDevice<F>(fsDev,0700,DEV_TYPE_FS,DEV_OPEN | DEV_READ | DEV_WRITE | DEV_CLOSE | DEV_DELEGATE),
		  _fs(fs), _clients(0) {
		this->set(MSG_FILE_OPEN,std::make_memfun(this,&FSDevice::devopen));
		this->set(MSG_FILE_CLOSE,std::make_memfun(this,&FSDevice::devclose),false);
		this->set(MSG_FS_OPEN,std::make_memfun(this,&FSDevice::open));
		this->set(MSG_FILE_READ,std::make_memfun(this,&FSDevice::read));
		this->set(MSG_FILE_WRITE,std::make_memfun(this,&FSDevice::write));
		this->set(MSG_FS_CLOSE,std::make_memfun(this,&FSDevice::close),false);
		this->set(MSG_FS_ISTAT,std::make_memfun(this,&FSDevice::istat));
		this->set(MSG_FS_SYNCFS,std::make_memfun(this,&FSDevice::syncfs));
		this->set(MSG_FS_LINK,std::make_memfun(this,&FSDevice::link));
		this->set(MSG_FS_UNLINK,std::make_memfun(this,&FSDevice::unlink));
		this->set(MSG_FS_RENAME,std::make_memfun(this,&FSDevice::rename));
		this->set(MSG_FS_MKDIR,std::make_memfun(this,&FSDevice::mkdir));
		this->set(MSG_FS_RMDIR,std::make_memfun(this,&FSDevice::rmdir));
		this->set(MSG_FS_CHMOD,std::make_memfun(this,&FSDevice::chmod));
		this->set(MSG_FS_CHOWN,std::make_memfun(this,&FSDevice::chown));
		this->set(MSG_FS_UTIME,std::make_memfun(this,&FSDevice::utime));
		this->set(MSG_FS_TRUNCATE,std::make_memfun(this,&FSDevice::truncate));
		this->set(MSG_FS_SYMLINK,std::make_memfun(this,&FSDevice::symlink));
	}

	virtual ~FSDevice() {
		_fs->sync();
	}

	void loop() {
		ulong buf[IPC_DEF_SIZE / sizeof(ulong)];
		while(1) {
			msgid_t mid;
			int fd = getwork(this->id(),&mid,buf,sizeof(buf),this->isStopped() ? GW_NOBLOCK : 0);
			if(EXPECT_FALSE(fd < 0)) {
				if(fd != -EINTR) {
					/* no requests anymore and we should shutdown? */
					if(this->isStopped())
						break;
					printe("getwork failed");
				}
				continue;
			}

			esc::IPCStream is(fd,buf,sizeof(buf),mid);
			this->handleMsg(mid,is);
		}
	}

	void devopen(esc::IPCStream &is) {
		_clients++;
		is << esc::FileOpen::Response::success(0) << esc::Reply();
	}

	void devclose(esc::IPCStream &is) {
		::close(is.fd());
		if(--_clients == 0)
			this->stop();
	}

	void open(esc::IPCStream &is) {
		char path[MAX_PATH_LEN];
		esc::FileOpen::Request r(path,sizeof(path));
		is >> r;

		F *file;
		esc::FileOpen::Result res;
		mode_t mode = S_IFREG | (r.mode & MODE_PERM);
		res.ino = _fs->open(&r.u,path,&res.sympos,r.root,r.flags,mode,is.fd(),&file);
		if(res.ino >= 0) {
			this->add(is.fd(),file);
			is << esc::FileOpen::Response::success(res) << esc::Reply();
		}
		else
			is << esc::FileOpen::Response::error(res.ino) << esc::Reply();
	}

	void read(esc::IPCStream &is) {
		F *file = (*this)[is.fd()];
		esc::FileRead::Request r;
		is >> r;

		if(file) {
			esc::DataBuf buf(r.count,file->shm(),r.shmemoff);
			ssize_t res = _fs->read(file,buf.data(),r.offset,r.count);

			is << esc::FileRead::Response::result(res) << esc::Reply();
			if(r.shmemoff == -1 && res > 0)
				is << esc::ReplyData(buf.data(),res);
		}
		else
			handleInfoRead(is,r);
	}

	void write(esc::IPCStream &is) {
		F *file = (*this)[is.fd()];
		esc::FileWrite::Request r;
		is >> r;

		ssize_t res = -ENOTSUP;
		if(file) {
			esc::DataBuf buf(r.count,file->shm(),r.shmemoff);
			if(r.shmemoff == -1)
				is >> esc::ReceiveData(buf.data(),r.count);

			res = _fs->write(file,buf.data(),r.offset,r.count);
		}
		is << esc::FileWrite::Response::result(res) << esc::Reply();
	}

	void close(esc::IPCStream &is) {
		F *file = (*this)[is.fd()];
		_fs->close(file);
		esc::ClientDevice<F>::close(is);
	}

	void istat(esc::IPCStream &is) {
		struct ::stat info;
		int res = _fs->stat((*this)[is.fd()],&info);
		is << esc::FSStat::Response(info,res) << esc::Reply();
	}

	void syncfs(esc::IPCStream &is) {
		_fs->sync();

		is << esc::FSSync::Response(0) << esc::Reply();
	}

	void link(esc::IPCStream &is) {
		char name[MAX_PATH_LEN];
		esc::FSLink::Request r(name,sizeof(name));
		is >> r;

		F *targetFile = (*this)[is.fd()];
		F *dirFile = (*this)[r.dirFd];

		int res = _fs->link(&r.u,targetFile,dirFile,r.name.str());
		is << esc::FSLink::Response(res) << esc::Reply();
	}

	void unlink(esc::IPCStream &is) {
		char name[MAX_PATH_LEN];
		esc::FSUnlink::Request r(name,sizeof(name));
		is >> r;

		F *dir = (*this)[is.fd()];

		int res = _fs->unlink(&r.u,dir,r.name.str());
		is << esc::FSUnlink::Response(res) << esc::Reply();
	}

	void rename(esc::IPCStream &is) {
		char oldName[MAX_PATH_LEN];
		char newName[MAX_PATH_LEN];
		esc::FSRename::Request r(oldName,sizeof(oldName),newName,sizeof(newName));
		is >> r;

		F *oldDir = (*this)[is.fd()];
		F *newDir = (*this)[r.newDirFd];

		int res = _fs->rename(&r.u,oldDir,r.oldName.str(),newDir,r.newName.str());
		is << esc::FSRename::Response(res) << esc::Reply();
	}

	void mkdir(esc::IPCStream &is) {
		char name[MAX_PATH_LEN];
		esc::FSMkdir::Request r(name,sizeof(name));
		is >> r;

		F *file = (*this)[is.fd()];

		int res = _fs->mkdir(&r.u,file,r.name.str(),r.mode);
		is << esc::FSMkdir::Response(res) << esc::Reply();
	}

	void rmdir(esc::IPCStream &is) {
		char name[MAX_PATH_LEN];
		esc::FSRmdir::Request r(name,sizeof(name));
		is >> r;

		F *file = (*this)[is.fd()];

		int res = _fs->rmdir(&r.u,file,r.name.str());
		is << esc::FSRmdir::Response(res) << esc::Reply();
	}

	void symlink(esc::IPCStream &is) {
		char target[MAX_PATH_LEN];
		char name[MAX_PATH_LEN];
		esc::FSSymlink::Request r(name,sizeof(name),target,sizeof(target));
		is >> r;

		F *file = (*this)[is.fd()];

		int res = _fs->symlink(&r.u,file,r.name.str(),r.target.str());
		is << esc::FSSymlink::Response(res) << esc::Reply();
	}

	void chmod(esc::IPCStream &is) {
		esc::FSChmod::Request r;
		is >> r;

		F *file = (*this)[is.fd()];

		int res = _fs->chmod(&r.u,file,r.mode);
		is << esc::FSChmod::Response(res) << esc::Reply();
	}

	void chown(esc::IPCStream &is) {
		esc::FSChown::Request r;
		is >> r;

		F *file = (*this)[is.fd()];

		int res = _fs->chown(&r.u,file,r.uid,r.gid);
		is << esc::FSChown::Response(res) << esc::Reply();
	}

	void utime(esc::IPCStream &is) {
		esc::FSUtime::Request r;
		is >> r;

		F *file = (*this)[is.fd()];

		int res = _fs->utime(&r.u,file,&r.time);
		is << esc::FSUtime::Response(res) << esc::Reply();
	}

	void truncate(esc::IPCStream &is) {
		esc::FSTruncate::Request r;
		is >> r;

		F *file = (*this)[is.fd()];

		int res = _fs->truncate(&r.u,file,r.length);
		is << esc::FSTruncate::Response(res) << esc::Reply();
	}

private:
	void handleInfoRead(esc::IPCStream &is,const esc::FileRead::Request &r) {
		FILE *str = fopendyn();
		char *data = NULL;
		ssize_t res = -ENOMEM;

		if(str) {
			_fs->print(str);
			data = fgetbuf(str,NULL);
			size_t len = strlen(data);
			if(r.offset >= len)
				res = 0;
			else if(r.offset + r.count > len)
				res = len - r.offset;
			else
				res = r.count;
			data += r.offset;
		}

		is << esc::FileRead::Response::result(res) << esc::Reply();
		if(res > 0)
			is << esc::ReplyData(data,res);
		if(str)
			fclose(str);
	}

	FileSystem<F> *_fs;
	size_t _clients;
};

}
