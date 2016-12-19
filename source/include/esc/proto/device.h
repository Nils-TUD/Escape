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
#include <sys/common.h>
#include <sys/messages.h>

namespace esc {

/**
 * The MSG_DEV_CANCEL command that is sent by the kernel to devices if cancel() was called on them.
 */
struct DevCancel {
	static const msgid_t MSG = MSG_DEV_CANCEL;

	struct Request {
		explicit Request() {
		}
		explicit Request(msgid_t _mid) : mid(_mid) {
		}

		union {
			struct {
				uint16_t msg;
				uint16_t id;
			};
			msgid_t mid;
		};
	};

	enum Result {
		/* request has been canceled */
		CANCELED,
		/* response can be retrieved */
		READY,
	};

	typedef ErrorResponse Response;
};

/**
 * The MSG_DEV_SHFILE command that is sent by the kernel to devices if shfile() was called on them.
 */
struct DevShFile {
	static const msgid_t MSG = MSG_DEV_SHFILE;

	struct Request {
		explicit Request() {
		}
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

	typedef ErrorResponse Response;
};

/**
 * The MSG_DEV_CREATSIBL command that is sent by the kernel to devices if creatsibl() was called
 * on them.
 */
struct DevCreatSibl {
	static const msgid_t MSG = MSG_DEV_CREATSIBL;

	struct Request {
		explicit Request() {
		}
		explicit Request(int _nfd,int _arg) : nfd(_nfd), arg(_arg) {
		}

		int nfd;
		int arg;
	};

	typedef ErrorResponse Response;
};

/**
 * The MSG_DEV_DELEGATE command that is sent by the kernel to devices if delegate() was called
 * on them.
 */
struct DevDelegate {
	static const msgid_t MSG = MSG_DEV_DELEGATE;

	struct Request {
		explicit Request() {
		}
		explicit Request(int _nfd,int _arg) : nfd(_nfd), arg(_arg) {
		}

		int nfd;
		int arg;
	};

	typedef ErrorResponse Response;
};

/**
 * The MSG_DEV_OBTAIN command that is sent by the kernel to devices if obtain() was called
 * on them.
 */
struct DevObtain {
	static const msgid_t MSG = MSG_DEV_OBTAIN;

	struct Request {
		explicit Request() {
		}
		explicit Request(int _arg) : arg(_arg) {
		}

		int arg;
	};

	struct Response : public ErrorResponse {
		explicit Response() : ErrorResponse(), fd(), perm() {
		}
		explicit Response(int _fd,uint _perm,errcode_t err)
			: ErrorResponse(err), fd(_fd), perm(_perm) {
		}

		static Response success(int fd,uint perm) {
			return Response(fd,perm,0);
		}
		static Response error(errcode_t err) {
			return Response(0,0,err);
		}
		static Response result(int res,uint perm) {
			return Response(res,perm,res < 0 ? res : 0);
		}

		int fd;
		uint perm;
	};
};

}
