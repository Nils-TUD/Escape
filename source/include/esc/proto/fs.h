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
#include <esc/proto/file.h>
#include <fs/filesystem.h>
#include <sys/common.h>
#include <sys/messages.h>

namespace esc {

struct FSOpen {
	static const msgid_t MSG = MSG_FS_OPEN;

	typedef FileOpen::Request Request;
	typedef FileOpen::Response Response;
};

struct FSRead {
	static const msgid_t MSG = MSG_FS_READ;

	typedef FileRead::Request Request;
	typedef FileRead::Response Response;
};

struct FSWrite {
	static const msgid_t MSG = MSG_FS_WRITE;

	typedef FileWrite::Request Request;
	typedef FileWrite::Response Response;
};

struct FSClose {
	static const msgid_t MSG = MSG_FS_CLOSE;

	typedef FileClose::Request Request;
};

struct FSSync {
	static const msgid_t MSG = MSG_FS_SYNCFS;

	typedef EmptyRequest Request;
	typedef ErrorResponse Response;
};

struct NameRequest {
	explicit NameRequest() {
	}
	explicit NameRequest(char *buffer,size_t _size) : name(buffer,_size) {
	}
	explicit NameRequest(const CString &_name) : name(_name) {
	}

	friend IPCBuf &operator<<(IPCBuf &is,const NameRequest &r) {
		return is << r.name;
	}
	friend IPCStream &operator<<(IPCStream &is,const NameRequest &r) {
		return is << r.name;
	}
	friend IPCStream &operator>>(IPCStream &is,NameRequest &r) {
		return is >> r.name;
	}

	CString name;
};

struct FSStat {
	static const msgid_t MSG = MSG_FS_ISTAT;

	typedef EmptyRequest Request;
	typedef ValueResponse<struct stat> Response;
};

struct FSLink {
	static const msgid_t MSG = MSG_FS_LINK;

	struct Request {
		explicit Request() {
		}
		explicit Request(char *buffer,size_t _size) : dirFd(), name(buffer,_size) {
		}
		explicit Request(int _dirFd,const CString &_name) : dirFd(_dirFd), name(_name) {
		}

		friend IPCBuf &operator<<(IPCBuf &is,const Request &r) {
			return is << r.dirFd << r.name;
		}
		friend IPCStream &operator<<(IPCStream &is,const Request &r) {
			return is << r.dirFd << r.name;
		}
		friend IPCStream &operator>>(IPCStream &is,Request &r) {
			return is >> r.dirFd >> r.name;
		}

		int dirFd;
		CString name;
	};

	typedef ErrorResponse Response;
};

struct FSUnlink {
	static const msgid_t MSG = MSG_FS_UNLINK;

	typedef NameRequest Request;
	typedef ErrorResponse Response;
};

struct FSRename {
	static const msgid_t MSG = MSG_FS_RENAME;

	struct Request {
		explicit Request() {
		}
		explicit Request(char *oldBuf,size_t oldSize,char *newBuf,size_t newSize)
			: oldName(oldBuf,oldSize), newDirFd(), newName(newBuf,newSize) {
		}
		explicit Request(const CString &_oldName,int _newDirFd,const CString &_newName)
			: oldName(_oldName), newDirFd(_newDirFd), newName(_newName) {
		}

		friend IPCBuf &operator<<(IPCBuf &is,const Request &r) {
			return is << r.oldName << r.newDirFd << r.newName;
		}
		friend IPCStream &operator<<(IPCStream &is,const Request &r) {
			return is << r.oldName << r.newDirFd << r.newName;
		}
		friend IPCStream &operator>>(IPCStream &is,Request &r) {
			return is >> r.oldName >> r.newDirFd >> r.newName;
		}

		CString oldName;
		int newDirFd;
		CString newName;
	};

	typedef ErrorResponse Response;
};

struct FSMkdir {
	static const msgid_t MSG = MSG_FS_MKDIR;

	struct Request {
		explicit Request() {
		}
		explicit Request(char *buffer,size_t _size) : mode(), name(buffer,_size) {
		}
		explicit Request(const CString &_name,mode_t _mode) : mode(_mode), name(_name) {
		}

		friend IPCBuf &operator<<(IPCBuf &is,const Request &r) {
			return is << r.mode << r.name;
		}
		friend IPCStream &operator<<(IPCStream &is,const Request &r) {
			return is << r.mode << r.name;
		}
		friend IPCStream &operator>>(IPCStream &is,Request &r) {
			return is >> r.mode >> r.name;
		}

		mode_t mode;
		CString name;
	};

	typedef ErrorResponse Response;
};

struct FSRmdir {
	static const msgid_t MSG = MSG_FS_RMDIR;

	typedef NameRequest Request;
	typedef ErrorResponse Response;
};

struct FSSymlink {
	static const msgid_t MSG = MSG_FS_SYMLINK;

	struct Request {
		explicit Request() {
		}
		explicit Request(char *nbuf,size_t nsize,char *tbuf,size_t tsize)
			: name(nbuf,nsize), target(tbuf,tsize) {
		}
		explicit Request(const CString &_name, const CString &_target)
			: name(_name), target(_target) {
		}

		friend IPCBuf &operator<<(IPCBuf &is,const Request &r) {
			return is << r.name << r.target;
		}
		friend IPCStream &operator<<(IPCStream &is,const Request &r) {
			return is << r.name << r.target;
		}
		friend IPCStream &operator>>(IPCStream &is,Request &r) {
			return is >> r.name >> r.target;
		}

		CString name;
		CString target;
	};
	typedef ErrorResponse Response;
};

struct FSChmod {
	static const msgid_t MSG = MSG_FS_CHMOD;

	struct Request {
		explicit Request() {
		}
		explicit Request(mode_t _mode) : mode(_mode) {
		}

		mode_t mode;
	};

	typedef ErrorResponse Response;
};

struct FSChown {
	static const msgid_t MSG = MSG_FS_CHOWN;

	struct Request {
		explicit Request() {
		}
		explicit Request(uid_t _uid,gid_t _gid) : uid(_uid), gid(_gid) {
		}

		uid_t uid;
		gid_t gid;
	};

	typedef ErrorResponse Response;
};

struct FSUtime {
	static const msgid_t MSG = MSG_FS_UTIME;

	struct Request {
		explicit Request() {
		}
		explicit Request(const utimbuf &_time) : time(_time) {
		}

		struct utimbuf time;
	};

	typedef ErrorResponse Response;
};

struct FSTruncate {
	static const msgid_t MSG = MSG_FS_TRUNCATE;

	struct Request {
		explicit Request() {
		}
		explicit Request(off_t _length) : length(_length) {
		}

		off_t length;
	};

	typedef ErrorResponse Response;
};

}
