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

UDP::socket_map UDP::_socks;

ssize_t UDP::send(const ipc::Net::IPv4Addr &ip,ipc::port_t srcp,ipc::port_t dstp,
		const void *data,size_t nbytes) {
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
	udp->checksum = genChecksum(route->link->ip(),ip,
		reinterpret_cast<uint16_t*>(udp),sizeof(UDP) + nbytes);

	ssize_t res = IPv4<UDP>::sendOver(route,pkt,total,ip,IP_PROTO);
	free(pkt);
	return res;
}

ssize_t UDP::receive(Link&,const Packet &packet) {
	const Ethernet<IPv4<UDP>> *pkt = packet.data<const Ethernet<IPv4<UDP>>*>();
	const UDP *udp = &pkt->payload.payload;
	socket_map::iterator it = _socks.find(be16tocpu(udp->dstPort));
	if(it != _socks.end()) {
		ipc::Socket::Addr sa;
		sa.family = ipc::Socket::AF_INET;
		sa.d.ipv4.addr = pkt->payload.src.value();
		sa.d.ipv4.port = be16tocpu(udp->srcPort);
		size_t offset = reinterpret_cast<const uint8_t*>(udp + 1) - packet.data<uint8_t*>();
		it->second->push(sa,packet,offset);
	}
	return 0;
}

uint16_t UDP::genChecksum(const ipc::Net::IPv4Addr &src,const ipc::Net::IPv4Addr &dst,
		const uint16_t *header,size_t sz) {
	struct {
		ipc::Net::IPv4Addr src;
		ipc::Net::IPv4Addr dst;
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
