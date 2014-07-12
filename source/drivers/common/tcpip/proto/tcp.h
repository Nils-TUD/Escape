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

#include <sys/common.h>
#include <sys/endian.h>
#include <map>

#include "../socket/streamsocket.h"
#include "../common.h"
#include "../link.h"
#include "../portmng.h"

#define DEBUG_TCP	0

#if DEBUG_TCP
#	define PRINT_TCP(locp,remp,fmt,...)	print("%5d,%5d: " fmt,locp,remp,##__VA_ARGS__)
#else
#	define PRINT_TCP(...)
#endif

class TCP {
	friend class StreamSocket;

	typedef std::map<uint32_t,StreamSocket*> socket_map;

public:
	enum {
		IP_PROTO     	= 6
	};

	enum {
		FL_FIN	= 1,
		FL_SYN	= 2,
		FL_RST	= 4,
		FL_PSH	= 8,
		FL_ACK	= 16,
		FL_URG	= 32,
	};

	size_t size() const {
		return sizeof(TCP);
	}

	static ssize_t send(const ipc::Net::IPv4Addr &ip,ipc::port_t srcp,ipc::port_t dstp,uint8_t flags,
		const void *data,size_t nbytes,size_t optSize,uint32_t seqNo,uint32_t ackNo,uint16_t winSize);
	static ssize_t receive(const std::shared_ptr<Link> &link,const Packet &packet);
	static void replyReset(const Ethernet<IPv4<TCP>> *pkt);

	static const char *flagsToStr(uint8_t flags);
	static void printSockets(std::ostream &os);

private:
	static ssize_t sendWith(Ethernet<IPv4<TCP>> *pkt,const ipc::Net::IPv4Addr &ip,ipc::port_t srcp,
		ipc::port_t dstp,uint8_t flags,const void *data,size_t nbytes,size_t optSize,uint32_t seqNo,
		uint32_t ackNo,uint16_t winSize);

	static uint32_t getKey(ipc::port_t localPort,ipc::port_t remotePort) {
		return ((uint32_t)localPort << 16) | remotePort;
	}
	static ssize_t addSocket(StreamSocket *sock,ipc::port_t localPort,ipc::port_t remotePort) {
		uint32_t key = getKey(localPort,remotePort);
		socket_map::iterator it = _socks.find(key);
		if(it != _socks.end())
			return it->second != sock ? -EADDRINUSE : 0;
		_socks[key] = sock;
		return 0;
	}
	static void remSocket(StreamSocket *sock,ipc::port_t localPort,ipc::port_t remotePort) {
		uint32_t key = getKey(localPort,remotePort);
		socket_map::iterator it = _socks.find(key);
		if(it != _socks.end() && it->second == sock)
			_socks.erase(it);
	}

public:
    ipc::port_t srcPort;
    ipc::port_t dstPort;
    uint32_t seqNumber;
    uint32_t ackNumber;
    uint8_t dataOffset;
    uint8_t ctrlFlags;
    uint16_t windowSize;
    uint16_t checksum;
    uint16_t urgentPtr;
	static socket_map _socks;
} A_PACKED;

static inline std::ostream &operator<<(std::ostream &os,const TCP &p) {
	os << "TCP payload:\n";
	os << "  srcPort    = " << be16tocpu(p.srcPort) << "\n";
	os << "  dstPort    = " << be16tocpu(p.dstPort) << "\n";
	os << "  seqNumber  = " << be32tocpu(p.seqNumber) << "\n";
	os << "  ackNumber  = " << be32tocpu(p.ackNumber) << "\n";
	os << "  dataOffset = " << p.dataOffset << "\n";
	os << "  ctrlFlags  = " << TCP::flagsToStr(p.ctrlFlags) << "\n";
	os << "  windowSize = " << be16tocpu(p.windowSize) << "\n";
	os << "  checksum   = " << be16tocpu(p.checksum) << "\n";
	os << "  urgentPtr  = " << be16tocpu(p.urgentPtr) << "\n";
	return os;
}
