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
#include <esc/endian.h>
#include <map>

#include "../socket/udpsocket.h"
#include "../common.h"
#include "../link.h"
#include "../portmng.h"

class UDP {
	friend class UDPSocket;

	typedef std::map<ipc::port_t,UDPSocket*> socket_map;

public:
	enum {
		IP_PROTO     	= 17
	};

	size_t size() const {
		return sizeof(UDP);
	}

	static ssize_t send(const ipc::Net::IPv4Addr &ip,ipc::port_t srcp,ipc::port_t dstp,
		const void *data,size_t nbytes);
	static ssize_t receive(Link &link,Ethernet<IPv4<UDP>> *packet,size_t sz);

private:
	static ssize_t addSocket(UDPSocket *sock,ipc::port_t port) {
		socket_map::iterator it = _socks.find(port);
		if(it != _socks.end())
			return it->second != sock ? -EADDRINUSE : 0;
		_socks[port] = sock;
		return 0;
	}
	static void remSocket(UDPSocket *sock,ipc::port_t port) {
		socket_map::iterator it = _socks.find(port);
		if(it != _socks.end() && it->second == sock)
			_socks.erase(it);
	}
	static uint16_t genChecksum(const ipc::Net::IPv4Addr &src,const ipc::Net::IPv4Addr &dst,
		const uint16_t *header,size_t sz);

public:
    ipc::port_t srcPort;
    ipc::port_t dstPort;
    uint16_t dataSize;
    uint16_t checksum;
	static socket_map _socks;
} A_PACKED;

static inline std::ostream &operator<<(std::ostream &os,const UDP &p) {
	os << "  UDP payload:\n";
	os << "  srcPort = " << be16tocpu(p.srcPort) << "\n";
	os << "  dstPort = " << be16tocpu(p.dstPort) << "\n";
	os << "  dataSize = " << be16tocpu(p.dataSize) << "\n";
	return os;
}
