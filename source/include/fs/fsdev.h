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

#include <sys/common.h>
#include <esc/ipc/clientdevice.h>
#include <fs/common.h>
#include <fs/infodev.h>
#include <stdio.h>

class FileSystem;

struct OpenFile : public esc::Client {
public:
	explicit OpenFile(int fd) : Client(fd), ino() {
	}
	explicit OpenFile(int fd,ino_t _ino) : Client(fd), ino(_ino) {
	}

	ino_t ino;
};

class FSDevice : public esc::ClientDevice<OpenFile> {
public:
	static FSDevice *getInstance() {
		return _inst;
	}

	explicit FSDevice(FileSystem *fs,const char *diskDev);
	virtual ~FSDevice();

	void loop();

	void devopen(esc::IPCStream &is);
	void devclose(esc::IPCStream &is);
	void open(esc::IPCStream &is);
	void read(esc::IPCStream &is);
	void write(esc::IPCStream &is);
	void close(esc::IPCStream &is);
	void stat(esc::IPCStream &is);
	void istat(esc::IPCStream &is);
	void syncfs(esc::IPCStream &is);
	void link(esc::IPCStream &is);
	void unlink(esc::IPCStream &is);
	void rename(esc::IPCStream &is);
	void mkdir(esc::IPCStream &is);
	void rmdir(esc::IPCStream &is);
	void chmod(esc::IPCStream &is);
	void chown(esc::IPCStream &is);

private:
	const char *resolveDir(FSUser *u,char *path,ino_t *ino);

	FileSystem *_fs;
	InfoDevice _info;
	size_t _clients;
	static FSDevice *_inst;
};
