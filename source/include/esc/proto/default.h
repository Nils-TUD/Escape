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
#include <sys/common.h>
#include <sys/messages.h>

namespace esc {

struct EmptyRequest {
};

struct ErrorResponse {
	explicit ErrorResponse() : err() {
	}
	explicit ErrorResponse(errcode_t _err) : err(_err) {
	}

	friend IPCBuf &operator>>(IPCBuf &is,ErrorResponse &r) {
		is >> r.err;
		if(is.error())
			r.err = -EINVAL;
		return is;
	}
	friend IPCStream &operator>>(IPCStream &is,ErrorResponse &r) {
		is >> r.err;
		if(is.error())
			r.err = -EINVAL;
		return is;
	}
	friend IPCStream &operator<<(IPCStream &is,const ErrorResponse &r) {
		return is << r.err;
	}

	errcode_t err;
};

template<typename T = void>
struct ValueResponse : public ErrorResponse {
	explicit ValueResponse() : ErrorResponse(), res() {
	}
	explicit ValueResponse(const T &_res, errcode_t err) : ErrorResponse(err), res(_res) {
	}

	static ValueResponse success(const T &res) {
		return ValueResponse(res,0);
	}
	static ValueResponse error(errcode_t err) {
		return ValueResponse(0,err);
	}
	static ValueResponse result(const T &res) {
		return ValueResponse(res,res < 0 ? res : 0);
	}

	friend IPCBuf &operator>>(IPCBuf &is,ValueResponse &r) {
		is >> r.err >> r.res;
		if(is.error())
			r.res = -EINVAL;
		return is;
	}
	friend IPCStream &operator>>(IPCStream &is,ValueResponse &r) {
		is >> r.err >> r.res;
		if(is.error())
			r.res = -EINVAL;
		return is;
	}
	friend IPCStream &operator<<(IPCStream &is,const ValueResponse &r) {
		return is << r.err << r.res;
	}

	T res;
};

}
