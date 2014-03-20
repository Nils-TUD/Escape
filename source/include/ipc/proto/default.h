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

namespace ipc {

template<class T,msgid_t M>
struct SimpleRequest {
	static const msgid_t MID = M;

	friend IPCStream &operator<<(IPCStream &is,const T &r) {
		return is << r << Send(MID);
	}
	friend IPCStream &operator>>(IPCStream &is,T &r) {
		return is >> r;
	}
};

template<msgid_t MID>
struct EmptyRequest {
	friend IPCStream &operator<<(IPCStream &is,const EmptyRequest &) {
		is << Send(MID);
		return is;
	}
};

template<class T>
struct SimpleResponse {
	friend IPCStream &operator<<(IPCStream &is,const T &r) {
		return is << r << Send(MSG_DEF_RESPONSE);
	}
	friend IPCStream &operator>>(IPCStream &is,T &r) {
		return is >> Receive() >> r;
	}
};

template<typename T,msgid_t MID>
struct DefaultResponse {
	explicit DefaultResponse() : res() {
	}
	explicit DefaultResponse(T _res) : res(_res) {
	}

	friend IPCBuf &operator>>(IPCBuf &is,DefaultResponse &r) {
		is >> r.res;
		if(is.error())
			r.res = -EINVAL;
		return is;
	}
	friend IPCStream &operator>>(IPCStream &is,DefaultResponse &r) {
		is >> r.res;
		if(is.error())
			r.res = -EINVAL;
		return is;
	}
	friend IPCStream &operator<<(IPCStream &is,const DefaultResponse &r) {
		is << r.res << Send(MID);
		return is;
	}

	T res;
};

}
