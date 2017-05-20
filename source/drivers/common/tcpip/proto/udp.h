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

#include <esc/stream/ostream.h>
#include <sys/common.h>
#include <sys/endian.h>
#include <map>

#include "../socket/dgramsocket.h"
#include "../common.h"
#include "../link.h"
#include "../portmng.h"

class UDP {
	friend class DGramSocket;

	typedef std::map<esc::port_t,DGramSocket*> socket_map;

public:
	enum {
		IP_PROTO     	= 17
	};

	size_t size() const {
		return sizeof(UDP);
	}

	static ssize_t send(const esc::Net::IPv4Addr &ip,esc::port_t srcp,esc::port_t dstp,
		const void *data,size_t nbytes);
	static ssize_t receive(const std::shared_ptr<Link> &link,const Packet &packet);

	static void printSockets(esc::OStream &os);

private:
	static ssize_t addSocket(DGramSocket *sock,esc::port_t port) {
		socket_map::iterator it = _socks.find(port);
		if(it != _socks.end())
			return it->second != sock ? -EADDRINUSE : 0;
		_socks[port] = sock;
		return 0;
	}
	static void remSocket(DGramSocket *sock,esc::port_t port) {
		socket_map::iterator it = _socks.find(port);
		if(it != _socks.end() && it->second == sock)
			_socks.erase(it);
	}

public:
    esc::port_t srcPort;
    esc::port_t dstPort;
    uint16_t dataSize;
    uint16_t checksum;

private:
	static socket_map _socks;
} A_PACKED A_ALIGNED(2);

static inline esc::OStream &operator<<(esc::OStream &os,const UDP &p) {
	os << "UDP payload:\n";
	os << "  srcPort    = " << be16tocpu(p.srcPort) << "\n";
	os << "  dstPort    = " << be16tocpu(p.dstPort) << "\n";
	os << "  dataSize   = " << be16tocpu(p.dataSize) << "\n";
	return os;
}
