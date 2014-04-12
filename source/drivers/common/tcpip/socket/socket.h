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
#include <esc/mem.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <ipc/clientdevice.h>
#include <assert.h>
#include <string.h>
#include <list>

#include "../common.h"

class Socket : public ipc::Client {
public:
	struct Packet {
		const void *data;
		size_t size;
		ipc::Socket::Addr sa;

		explicit Packet(const void *_data,size_t _size,const ipc::Socket::Addr &_sa)
			: data(_data), size(_size), sa(_sa) {
		}
	};

	explicit Socket(int f) : ipc::Client(f), _pending() {
	}
	virtual ~Socket() {
	}

	virtual int bind(const ipc::Socket::Addr *) {
		return -ENOTSUP;
	}
	virtual ssize_t sendto(const ipc::Socket::Addr *,const void *,size_t) {
		return -ENOTSUP;
	}
	virtual ssize_t recvfrom(bool needsSockAddr,void *buffer,size_t size) {
		if(_packets.size() > 0) {
			Packet pkt = _packets.front();
			if(pkt.size > size)
				return -ENOMEM;
			reply(pkt.sa,needsSockAddr,buffer,pkt.data,size);
			_packets.pop_front();
			return 0;
		}

		if(_pending.data)
			return -EAGAIN;
		_pending.data = buffer;
		_pending.count = size;
		_pending.needsSockAddr = needsSockAddr;
		return 0;
	}

	void push(const ipc::Socket::Addr &sa,const void *data,size_t size) {
		if(_pending.count >= size) {
			reply(sa,_pending.needsSockAddr,_pending.data,data,size);
			_pending.count = 0;
		}
		else
			_packets.push_back(Packet(data,size,sa));
	}

private:
	void reply(const ipc::Socket::Addr &sa,bool needsSockAddr,void *dst,const void *src,size_t size) {
		if(dst != NULL)
			memcpy(dst,src,size);

		ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
		ipc::IPCStream is(fd(),buffer,sizeof(buffer));
		is << ipc::FileRead::Response(size);
		if(needsSockAddr)
			is << sa;
		is << ipc::Send(ipc::FileRead::Response::MID);
		if(dst == NULL)
			is << ipc::SendData(ipc::FileRead::Response::MID,src,size);
	}

	ReadRequest _pending;
	std::list<Packet> _packets;
};
