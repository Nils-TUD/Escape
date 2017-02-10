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

#include <esc/ipc/ipcstream.h>
#include <esc/proto/default.h>
#include <fs/filesystem.h>
#include <sys/common.h>
#include <sys/messages.h>

namespace esc {

/**
 * The MSG_FILE_OPEN command that is sent by the kernel to devices if open() was called on them.
 */
struct FileOpen {
	static const msgid_t MSG = MSG_FILE_OPEN;

	struct Request {
		explicit Request() {
		}
		explicit Request(char *buffer,size_t size) : path(buffer,size) {
		}
		explicit Request(uint _flags,const fs::User &_u,const CString &_path,ino_t _root,mode_t _mode)
			: flags(_flags), u(_u), path(_path), root(_root), mode(_mode) {
		}

		friend IPCBuf &operator<<(IPCBuf &ib,const Request &r) {
			return ib << r.flags << r.u.uid << r.u.gid << r.u.pid << r.path << r.root << r.mode;
		}
		friend IPCStream &operator>>(IPCStream &is,Request &r) {
			return is >> r.flags >> r.u.uid >> r.u.gid >> r.u.pid >> r.path >> r.root >> r.mode;
		}

		uint flags;
		fs::User u;
		CString path;
		ino_t root;
		mode_t mode;
	};

	typedef ValueResponse<ino_t> Response;
};

/**
 * The MSG_FILE_READ command that is sent by the kernel to devices if read() was called on them.
 */
struct FileRead {
	static const msgid_t MSG = MSG_FILE_READ;

	struct Request {
		explicit Request() {
		}
		explicit Request(size_t _offset,size_t _count,ssize_t _shmemoff)
			: offset(_offset), count(_count), shmemoff(_shmemoff) {
		}

		size_t offset;
		size_t count;
		ssize_t shmemoff;
	};

	typedef ValueResponse<size_t> Response;
};

/**
 * The MSG_FILE_WRITE command that is sent by the kernel to devices if write() was called on them.
 */
struct FileWrite {
	static const msgid_t MSG = MSG_FILE_WRITE;

	struct Request {
		explicit Request() {
		}
		explicit Request(size_t _offset,size_t _count,ssize_t _shmemoff)
			: offset(_offset), count(_count), shmemoff(_shmemoff) {
		}

		size_t offset;
		size_t count;
		ssize_t shmemoff;
	};

	typedef ValueResponse<size_t> Response;
};

/**
 * The MSG_FILE_SIZE command that is sent by the kernel to devices if filesize() was called on them.
 */
struct FileSize {
	static const msgid_t MSG = MSG_FILE_SIZE;

	typedef EmptyRequest Request;
	typedef ValueResponse<size_t> Response;
};

/**
 * The MSG_FILE_CLOSE command that is sent by the kernel to devices if close() was called on them.
 */
struct FileClose {
	static const msgid_t MSG = MSG_FILE_CLOSE;

	typedef EmptyRequest Request;
};

}
