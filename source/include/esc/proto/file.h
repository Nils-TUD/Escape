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
#include <sys/messages.h>
#include <esc/ipc/ipcstream.h>
#include <esc/proto/default.h>
#include <fs/filesystem.h>

namespace esc {

/**
 * The MSG_FILE_OPEN command that is sent by the kernel to devices if open() was called on them.
 */
struct FileOpen {
	struct Request {
		static const msgid_t MSG = MSG_FILE_OPEN;

		explicit Request(char *buffer,size_t size) : path(buffer,size) {
		}
		explicit Request(uint _flags,const FSUser &_u,const CString &_path,mode_t _mode)
			: flags(_flags), u(_u), path(_path), mode(_mode) {
		}

		friend IPCBuf &operator<<(IPCBuf &ib,const Request &r) {
			return ib << r.flags << r.u.uid << r.u.gid << r.u.pid << r.path << r.mode;
		}
		friend IPCStream &operator>>(IPCStream &is,Request &r) {
			return is >> r.flags >> r.u.uid >> r.u.gid >> r.u.pid >> r.path >> r.mode;
		}

		uint flags;
		FSUser u;
		CString path;
		mode_t mode;
	};

	typedef DefaultResponse<int> Response;
};

/**
 * The MSG_DEV_SHFILE command that is sent by the kernel to devices if shfile() was called on them.
 */
struct FileShFile {
	struct Request {
		static const msgid_t MSG = MSG_DEV_SHFILE;

		explicit Request(char *buffer,size_t _size) : path(buffer,_size) {
		}
		explicit Request(size_t _size,const CString &_path)
			: size(_size), path(_path) {
		}

		friend IPCBuf &operator<<(IPCBuf &is,const Request &r) {
			return is << r.size << r.path;
		}
		friend IPCStream &operator<<(IPCStream &is,const Request &r) {
			return is << r.size << r.path;
		}
		friend IPCStream &operator>>(IPCStream &is,Request &r) {
			return is >> r.size >> r.path;
		}

		size_t size;
		CString path;
	};

	typedef DefaultResponse<int> Response;
};

/**
 * The MSG_DEV_CREATSIBL command that is sent by the kernel to devices if creatsibl() was called
 * on them.
 */
struct FileCreatSibl {
	struct Request {
		static const msgid_t MSG = MSG_DEV_CREATSIBL;

		explicit Request() {
		}
		explicit Request(int _nfd,int _arg) : nfd(_nfd), arg(_arg) {
		}

		friend IPCBuf &operator<<(IPCBuf &is,const Request &r) {
			return is << r.nfd << r.arg;
		}
		friend IPCStream &operator<<(IPCStream &is,const Request &r) {
			return is << r.nfd << r.arg;
		}
		friend IPCStream &operator>>(IPCStream &is,Request &r) {
			return is >> r.nfd >> r.arg;
		}

		int nfd;
		int arg;
	};

	typedef DefaultResponse<int> Response;
};

/**
 * The MSG_FILE_READ command that is sent by the kernel to devices if read() was called on them.
 */
struct FileRead {
	struct Request {
		static const msgid_t MSG = MSG_FILE_READ;

		explicit Request() {
		}
		explicit Request(size_t _offset,size_t _count,ssize_t _shmemoff)
			: offset(_offset), count(_count), shmemoff(_shmemoff) {
		}

		size_t offset;
		size_t count;
		ssize_t shmemoff;
	};

	typedef DefaultResponse<ssize_t> Response;
};

/**
 * The MSG_FILE_WRITE command that is sent by the kernel to devices if write() was called on them.
 */
struct FileWrite {
	struct Request {
		static const msgid_t MSG = MSG_FILE_WRITE;

		explicit Request() {
		}
		explicit Request(size_t _offset,size_t _count,ssize_t _shmemoff)
			: offset(_offset), count(_count), shmemoff(_shmemoff) {
		}

		size_t offset;
		size_t count;
		ssize_t shmemoff;
	};

	typedef DefaultResponse<ssize_t> Response;
};

/**
 * The MSG_FILE_SIZE command that is sent by the kernel to devices if filesize() was called on them.
 */
struct FileSize {
	typedef DefaultResponse<ssize_t> Response;
};

}
