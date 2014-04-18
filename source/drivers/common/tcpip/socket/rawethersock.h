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

#include "../common.h"
#include "socket.h"
#include "rawsocketlist.h"

class RawEtherSocket : public Socket {
public:
	explicit RawEtherSocket(int f,int proto) : Socket(f,proto) {
		if(proto != ipc::Socket::PROTO_IP && proto != ipc::Socket::PROTO_ANY)
			VTHROWE("A raw ethernet socket doesn't support protocol " << proto,-ENOTSUP);
	}
	virtual ~RawEtherSocket() {
		sockets.remove(this);
	}

	virtual int bind(const ipc::Socket::Addr *) {
		return sockets.add(this);
	}
	virtual ssize_t sendto(msgid_t,const ipc::Socket::Addr *,const void *,size_t ) {
		// TODO implement me
		return -ENOTSUP;
	}
	virtual ssize_t recvfrom(msgid_t mid,bool needsSockAddr,void *buffer,size_t size) {
		sockets.add(this);
		return Socket::recvfrom(mid,needsSockAddr,buffer,size);
	}

	static RawSocketList sockets;
};
