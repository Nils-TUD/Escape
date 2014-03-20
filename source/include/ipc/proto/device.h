/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/messages.h>
#include <ipc/ipcstream.h>
#include <ipc/proto/default.h>

namespace ipc {

struct DevOpen {
	struct Request {
		static const msgid_t MSG = MSG_DEV_OPEN;

		explicit Request(char *buffer,size_t size) : path(buffer,size) {
		}
		explicit Request(uint _flags,uid_t _uid,gid_t _gid,pid_t _pid,const CString &_path)
			: flags(_flags), uid(_uid), gid(_gid), pid(_pid), path(_path) {
		}

		friend IPCBuf &operator<<(IPCBuf &ib,const Request &r) {
			return ib << r.flags << r.uid << r.gid << r.pid << r.path;
		}
		friend IPCStream &operator>>(IPCStream &is,Request &r) {
			return is >> r.flags >> r.uid >> r.gid >> r.pid >> r.path;
		}

		uint flags;
		uid_t uid;
		gid_t gid;
		pid_t pid;
		CString path;
	};

	typedef DefaultResponse<int,MSG_DEV_OPEN_RESP> Response;
};

struct DevShFile {
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

	typedef DefaultResponse<int,MSG_DEV_SHFILE_RESP> Response;
};

struct DevRead {
	struct Request {
		static const msgid_t MSG = MSG_DEV_READ;

		explicit Request() {
		}
		explicit Request(size_t _offset,size_t _count,ssize_t _shmemoff)
			: offset(_offset), count(_count), shmemoff(_shmemoff) {
		}

		size_t offset;
		size_t count;
		ssize_t shmemoff;
	};

	struct Response {
		static const msgid_t MSG = MSG_DEV_READ_RESP;

		explicit Response() {
		}
		explicit Response(ssize_t count,void *buffer) : res(count), data(buffer) {
		}

		friend IPCStream &operator<<(IPCStream &is,const Response &r) {
			is << r.res << Send(MSG);
			if(r.res && r.data)
				is << SendData(MSG,r.data,r.res);
			return is;
		}
		friend IPCBuf &operator>>(IPCBuf &is,Response &r) {
			/* don't receive the data here, because this involves kernel-stuff */
			is >> r.res;
			if(is.error())
				r.res = -EINVAL;
			return is;
		}
		friend IPCStream &operator>>(IPCStream &is,Response &r) {
			size_t max = r.res;
			is >> r.res;
			if(r.res && r.data)
				is >> ReceiveData(r.data,max);
			if(is.error())
				r.res = -EINVAL;
			return is;
		}

		ssize_t res;
		void *data;
	};
};

typedef void *(*alloc_func)(size_t size);
typedef void (*free_func)(void *ptr);

static inline void *_alloc_dummy(size_t) {
	return NULL;
}
static inline void _free_dummy(void*) {
}

template<alloc_func ALLOC = _alloc_dummy,free_func FREE = _free_dummy>
struct DevWrite {
	struct Request {
		static const msgid_t MSG = MSG_DEV_WRITE;

		explicit Request() {
		}
		explicit Request(size_t _offset,size_t _count,ssize_t _shmemoff,void *_data = NULL)
			: offset(_offset), count(_count), shmemoff(_shmemoff), data(_data) {
		}
		~Request() {
			FREE(data);
		}

		friend IPCBuf &operator<<(IPCBuf &ib,const Request &r) {
			/* again no send for the kernel */
			return ib << r.offset << r.count << r.shmemoff;
		}
		friend IPCStream &operator<<(IPCStream &is,const Request &r) {
			return is << r.offset << r.count << r.shmemoff << Send(MSG) << SendData(MSG,r.data,r.count);
		}
		friend IPCStream &operator>>(IPCStream &is,Request &r) {
			is >> r.offset >> r.count >> r.shmemoff;
			if(r.shmemoff == -1) {
				r.data = ALLOC(r.count);
				is >> ReceiveData(r.data,r.count);
			}
			else
				r.data = NULL;
			return is;
		}

		size_t offset;
		size_t count;
		ssize_t shmemoff;
		void *data;
	};

	typedef DefaultResponse<ssize_t,MSG_DEV_WRITE_RESP> Response;
};

}
