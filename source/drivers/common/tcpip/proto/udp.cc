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

#include <sys/common.h>
#include <iostream>

#include "ethernet.h"
#include "ipv4.h"
#include "udp.h"

UDP::socket_map UDP::_socks;

ssize_t UDP::send(const esc::Net::IPv4Addr &ip,esc::port_t srcp,esc::port_t dstp,
		const void *data,size_t nbytes) {
	Route route = Route::find(ip);
	if(!route.valid())
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
	udp->checksum = esc::Net::ipv4PayloadChecksum(route.link->ip(),ip,IP_PROTO,
		reinterpret_cast<uint16_t*>(udp),sizeof(UDP) + nbytes);

	ssize_t res = IPv4<UDP>::sendOver(route,pkt,total,ip,IP_PROTO);
	free(pkt);
	return res;
}

ssize_t UDP::receive(const std::shared_ptr<Link>&,const Packet &packet) {
	const Ethernet<IPv4<UDP>> *pkt = packet.data<const Ethernet<IPv4<UDP>>*>();
	const UDP *udp = &pkt->payload.payload;
	socket_map::iterator it = _socks.find(be16tocpu(udp->dstPort));
	if(it != _socks.end()) {
		esc::Socket::Addr sa;
		sa.family = esc::Socket::AF_INET;
		sa.d.ipv4.addr = pkt->payload.src.value();
		sa.d.ipv4.port = be16tocpu(udp->srcPort);
		size_t offset = reinterpret_cast<const uint8_t*>(udp + 1) - packet.data<uint8_t*>();
		it->second->push(sa,packet,offset);
	}
	return 0;
}

void UDP::printSockets(std::ostream &os) {
	for(auto it = _socks.begin(); it != _socks.end(); ++it)
		os << it->second->fd() << " UDP *:" << it->first << "\n";
}
