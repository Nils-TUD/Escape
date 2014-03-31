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

ssize_t UDP::send(const IPv4Addr &ip,uint16_t srcp,uint16_t dstp,const void *data,size_t nbytes) {
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
