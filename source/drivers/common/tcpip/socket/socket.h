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
#include "../packet.h"

class Socket : public ipc::Client {
public:
	struct QueuedPacket {
		std::shared_ptr<PacketData> data;
		size_t offset;
		ipc::Socket::Addr sa;

		explicit QueuedPacket(std::shared_ptr<PacketData> _data,size_t _offset,
				const ipc::Socket::Addr &_sa)
			: data(_data), offset(_offset), sa(_sa) {
		}
	};

	explicit Socket(int f,int proto = ipc::Socket::PROTO_ANY)
		: ipc::Client(f), _proto(proto), _pending() {
	}
	virtual ~Socket() {
	}

	int protocol() const {
		return _proto;
	}

	virtual int cancel(msgid_t mid) {
		if(!_pending.count)
			return 1;
		if(_pending.mid != mid)
			return -EINVAL;
		_pending.count = 0;
		return 0;
	}

	virtual int bind(const ipc::Socket::Addr *) {
		return -ENOTSUP;
	}

	virtual ssize_t sendto(const ipc::Socket::Addr *,const void *,size_t) {
		return -ENOTSUP;
	}

	virtual ssize_t recvfrom(msgid_t mid,bool needsSrc,void *buffer,size_t size) {
		if(_packets.size() > 0) {
			const QueuedPacket &pkt = _packets.front();
			if(pkt.data->size - pkt.offset <= size) {
				reply(mid,pkt.sa,needsSrc,buffer,pkt.data->data + pkt.offset,pkt.data->size - pkt.offset);
				_packets.pop_front();
				return 0;
			}
		}

		if(_pending.data)
			return -EAGAIN;
		_pending.mid = mid;
		_pending.data = buffer;
		_pending.count = size;
		_pending.needsSrc = needsSrc;
		return 0;
	}

	void push(const ipc::Socket::Addr &sa,const Packet &pkt,size_t offset = 0) {
		if(_pending.count > 0) {
			if(_pending.count >= pkt.size() - offset) {
				reply(_pending.mid,sa,_pending.needsSrc,_pending.data,pkt.data<uint8_t*>() + offset,
					pkt.size() - offset);
				_pending.count = 0;
			}
		}
		else
			_packets.push_back(QueuedPacket(pkt.copy(),offset,sa));
	}

private:
	void reply(msgid_t mid,const ipc::Socket::Addr &sa,bool needsSrc,void *dst,const void *src,size_t size) {
		if(dst != NULL)
			memcpy(dst,src,size);

		ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
		ipc::IPCStream is(fd(),buffer,sizeof(buffer),mid);
		is << ipc::FileRead::Response(size);
		if(needsSrc)
			is << sa;
		is << ipc::Reply();
		if(dst == NULL)
			is << ipc::ReplyData(src,size);
	}

	int _proto;
	ReadRequest _pending;
	std::list<QueuedPacket> _packets;
};
