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

#include <esc/common.h>
#include <iostream>

#include "ethernet.h"
#include "udp.h"
#include "ipv4.h"

PortMng<PRIVATE_PORTS_CNT> UDPSocket::_ports(PRIVATE_PORTS);
UDP::socket_map UDP::_socks;

UDPSocket::UDPSocket(int fd,const IPv4Addr &ip,int port)
	: Socket(fd), _dstIp(ip), _srcPort(0), _dstPort(port) {
}

UDPSocket::~UDPSocket() {
	UDP::remSocket(this,_dstPort);
	if(_srcPort != 0) {
		UDP::remSocket(this,_srcPort);
		_ports.release(_srcPort);
	}
}

ssize_t UDPSocket::sendto(const sSockAddr *sa,const void *buffer,size_t size) {
	IPv4Addr addr = sa ? IPv4Addr(sa->d.ipv4.addr) : _dstIp;
	port_t port = sa ? sa->d.ipv4.port : _dstPort;

	// do we still need a local port?
	if(_srcPort == 0) {
		_srcPort = _ports.allocate();
		if(_srcPort == 0)
			return -EAGAIN;
		UDP::addSocket(this,_srcPort);
	}

	return UDP::send(addr,_srcPort,port,buffer,size);
}

ssize_t UDPSocket::receive(void *buffer,size_t size) {
	ssize_t res = UDP::addSocket(this,_dstPort);
	if(res < 0)
		return res;
	return Socket::receive(buffer,size);
}

ssize_t UDP::send(const IPv4Addr &ip,port_t srcp,port_t dstp,const void *data,size_t nbytes) {
	const Route *route = Route::find(ip);
	if(!route)
		return -ENETUNREACH;

	const size_t total = Ethernet<IPv4<UDP>>().size() + nbytes;
	Ethernet<IPv4<UDP>> *pkt = (Ethernet<IPv4<UDP>>*)malloc(total);
	if(!pkt)
		return -ENOMEM;

	UDP *udp = &pkt->payload.payload;
	udp->srcPort = cputobe16(srcp);
	udp->dstPort = cputobe16(dstp);
	udp->dataSize = cputobe16(sizeof(UDP) + nbytes);
	memcpy(udp + 1,data,nbytes);

	udp->checksum = 0;
	udp->checksum = genChecksum(route->nic->ip(),ip,
		reinterpret_cast<uint16_t*>(udp),sizeof(UDP) + nbytes);

	ssize_t res = IPv4<UDP>::sendOver(route,pkt,total,ip,IP_PROTO);
	free(pkt);
	return res;
}

ssize_t UDP::receive(NIC &,Ethernet<IPv4<UDP>> *packet,size_t) {
	const UDP *udp = &packet->payload.payload;
	socket_map::iterator it = _socks.find(be16tocpu(udp->dstPort));
	if(it != _socks.end())
		it->second->push(packet->payload.src,udp + 1,be16tocpu(udp->dataSize));
	return 0;
}

uint16_t UDP::genChecksum(const IPv4Addr &src,const IPv4Addr &dst,const uint16_t *header,size_t sz) {
	struct {
		IPv4Addr src;
		IPv4Addr dst;
		uint16_t proto;
		uint16_t dataSize;
	} A_PACKED pseudoHeader = {
		.src = src,
		.dst = dst,
		.proto = cputobe16(IP_PROTO),
		.dataSize = static_cast<uint16_t>(cputobe16(sz))
	};

	uint32_t checksum = 0;
	const uint16_t *data = reinterpret_cast<uint16_t*>(&pseudoHeader);
	for(size_t i = 0; i < sizeof(pseudoHeader) / 2; ++i)
		checksum += data[i];
	for(size_t i = 0; i < sz / 2; ++i)
		checksum += header[i];
	if((sz % 2) != 0)
		checksum += header[sz / 2] & 0xFF;

	while(checksum >> 16)
		checksum = (checksum & 0xFFFF) + (checksum >> 16);
	return ~checksum;
}
