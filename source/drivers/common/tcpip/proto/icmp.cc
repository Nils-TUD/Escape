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

#include "ethernet.h"
#include "icmp.h"
#include "ipv4.h"

ssize_t ICMP::send(const esc::Net::IPv4Addr &ip,const void *payload,size_t nbytes,
		uint8_t code,uint8_t type,uint16_t id,uint16_t seq) {
	if(nbytes > MAX_ECHO_PAYLOAD_SIZE)
		return -EINVAL;

	const size_t total = Ethernet<IPv4<ICMP>>().size() + nbytes;
	Ethernet<IPv4<ICMP>> *pkt = (Ethernet<IPv4<ICMP>>*)malloc(total);
	if(!pkt)
		return -ENOMEM;

	ICMP *icmp = &pkt->payload.payload;
	icmp->code = code;
	icmp->type = type;
	icmp->identifier = cputobe16(id);
	icmp->sequence = cputobe16(seq);
	memcpy(icmp + 1,payload,nbytes);

	icmp->checksum = 0;
	icmp->checksum = esc::Net::ipv4Checksum(reinterpret_cast<uint16_t*>(icmp),icmp->size() + nbytes);

	ssize_t res = IPv4<ICMP>::send(pkt,total,ip,IP_PROTO);
	free(pkt);
	return res;
}

ssize_t ICMP::handleEcho(const Ethernet<IPv4<ICMP>> *packet,size_t sz) {
	const ICMP *icmp = &packet->payload.payload;
	const char *payload = reinterpret_cast<const char*>(icmp + 1);
	size_t plsz = reinterpret_cast<const char*>(packet) + sz - payload;
	return send(packet->payload.src,payload,plsz,icmp->code,CMD_ECHO_REPLY,
		be16tocpu(icmp->identifier),be16tocpu(icmp->sequence));
}

ssize_t ICMP::receive(const std::shared_ptr<Link>&,const Packet &packet) {
	const Ethernet<IPv4<ICMP>> *pkt = packet.data<const Ethernet<IPv4<ICMP>>*>();
	const ICMP *icmp = &pkt->payload.payload;
	switch(icmp->type) {
		case CMD_ECHO_REPLY:
			return 0;

		case CMD_ECHO:
			return handleEcho(pkt,packet.size());
	}
	return -ENOTSUP;
}
