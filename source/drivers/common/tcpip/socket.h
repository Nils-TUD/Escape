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
#include <esc/net.h>
#include <esc/mem.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <assert.h>
#include <string.h>
#include <list>

#include "common.h"
#include "ipv4addr.h"

class Socket {
public:
	struct Packet {
		const void *data;
		size_t size;
		IPv4Addr ip;

		explicit Packet(const void *_data,size_t _size,const IPv4Addr &_ip)
			: data(_data), size(_size), ip(_ip) {
		}
	};

	explicit Socket(int fd) : _fd(fd), _shm(), _pending() {
	}
	virtual ~Socket() {
		if(_shm)
			munmap(_shm);
	}

	char *shm() {
		return _shm;
	}
	void shm(char *ptr) {
		assert(_shm == NULL);
		_shm = ptr;
	}

	virtual ssize_t sendto(const sSockAddr *sa,const void *buffer,size_t size) = 0;
	virtual ssize_t receive(void *buffer,size_t size) {
		if(_packets.size() > 0) {
			Packet pkt = _packets.front();
			if(pkt.size > size)
				return -ENOMEM;
			reply(pkt.ip,buffer,pkt.data,size);
			_packets.pop_front();
			return 0;
		}

		if(_pending.data)
			return -EAGAIN;
		_pending.data = buffer;
		_pending.count = size;
		return 0;
	}

	void push(const IPv4Addr &ip,const void *data,size_t size) {
		if(_pending.count >= size) {
			reply(ip,_pending.data,data,size);
			_pending.count = 0;
		}
		else
			_packets.push_back(Packet(data,size,ip));
	}

private:
	void reply(const IPv4Addr &ip,void *dst,const void *src,size_t size) {
		sArgsMsg msg;
		if(dst != NULL)
			memcpy(dst,src,size);
		msg.arg1 = size;
		msg.arg2 = ip.value();
		send(_fd,MSG_DEV_READ_RESP,&msg,sizeof(msg));
		if(dst == NULL)
			send(_fd,MSG_DEV_READ_RESP,src,size);
	}

	int _fd;
	char *_shm;
	ReadRequest _pending;
	std::list<Packet> _packets;
};
