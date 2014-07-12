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

#include <esc/common.h>
#include <ipc/clientdevice.h>
#include <fs/common.h>
#include <fs/infodev.h>
#include <stdio.h>

class FileSystem;

struct OpenFile : public ipc::Client {
public:
	explicit OpenFile(int fd) : Client(fd), ino() {
	}
	explicit OpenFile(int fd,ino_t _ino) : Client(fd), ino(_ino) {
	}

	ino_t ino;
};

class FSDevice : public ipc::ClientDevice<OpenFile> {
public:
	static FSDevice *getInstance() {
		return _inst;
	}

	explicit FSDevice(FileSystem *fs,const char *diskDev);
	virtual ~FSDevice();

	void loop();

	void devopen(ipc::IPCStream &is);
	void devclose(ipc::IPCStream &is);
	void open(ipc::IPCStream &is);
	void read(ipc::IPCStream &is);
	void write(ipc::IPCStream &is);
	void close(ipc::IPCStream &is);
	void stat(ipc::IPCStream &is);
	void istat(ipc::IPCStream &is);
	void syncfs(ipc::IPCStream &is);
	void link(ipc::IPCStream &is);
	void unlink(ipc::IPCStream &is);
	void rename(ipc::IPCStream &is);
	void mkdir(ipc::IPCStream &is);
	void rmdir(ipc::IPCStream &is);
	void chmod(ipc::IPCStream &is);
	void chown(ipc::IPCStream &is);

private:
	const char *resolveDir(FSUser *u,char *path,ino_t *ino);

	FileSystem *_fs;
	InfoDevice _info;
	size_t _clients;
	static FSDevice *_inst;
};
