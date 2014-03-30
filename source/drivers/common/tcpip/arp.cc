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
#include <esc/endian.h>
#include <iostream>

#include "arp.h"
#include "ethernet.h"
#include "ipv4.h"

ARP::pending_type ARP::_pending;
ARP::cache_type ARP::_cache;

int ARP::createPending(const void *packet,size_t size,const IPv4Addr &ip,uint16_t type) {
	Packet pkt;
	pkt.dest = ip;
	pkt.size = size;
	pkt.type = type;
	pkt.pkt = (Ethernet<>*)malloc(size);
	if(!pkt.pkt)
		return -ENOMEM;
	memcpy(pkt.pkt,packet,size);
	_pending.push_back(pkt);
	return 0;
}

void ARP::sendPending(NIC &nic) {
	for(auto it = _pending.begin(); it < _pending.end(); ) {
		cache_type::iterator entry = _cache.find(it->dest);
		if(entry != _cache.end()) {
			it->pkt->send(nic,entry->second,it->size,it->type);
			_pending.erase(it);
			free(it->pkt);
		}
		else
			it++;
	}
}

ssize_t ARP::requestMAC(NIC &nic,const IPv4Addr &ip) {
	Ethernet<ARP> pkt;
	ARP *arp = &pkt.payload;

	arp->hwAddrFmt = cputobe16(HW_ADDR_ETHER);
	arp->hwAddrSize = MACAddr::LEN;
	arp->protoAddrFmt = cputobe16(IPv4<>::ETHER_TYPE);
	arp->protoAddrSize = IPv4Addr::LEN;
	arp->cmd = cputobe16(CMD_REQUEST);

	arp->hwTarget = MACAddr();
	arp->ipTarget = ip;
	arp->hwSender = nic.mac();
	arp->ipSender = nic.ip();

	return pkt.send(nic,MACAddr::broadcast(),pkt.size(),ARP::ETHER_TYPE);
}

ssize_t ARP::handleRequest(NIC &nic,const ARP *packet) {
	// not a valid host in our network?
	if(!packet->ipSender.isHost(nic.subnetMask()))
		return -EINVAL;
	// TODO multicast is invalid too
	if(packet->hwSender == MACAddr::broadcast())
		return -EINVAL;

	// store the mapping in every case. perhaps we need it in future
	_cache[packet->ipSender] = packet->hwSender;

	// not for us?
	if(packet->ipTarget != nic.ip())
		return 0;

	// reply our MAC address to sender
	Ethernet<ARP> pkt;
	ARP *arp = &pkt.payload;

	arp->hwAddrFmt = cputobe16(HW_ADDR_ETHER);
	arp->hwAddrSize = MACAddr::LEN;
	arp->protoAddrFmt = cputobe16(IPv4<>::ETHER_TYPE);
	arp->protoAddrSize = IPv4Addr::LEN;
	arp->cmd = cputobe16(CMD_REPLY);

	arp->hwTarget = packet->hwSender;
	arp->hwSender = nic.mac();
	arp->ipTarget = packet->ipSender;
	arp->ipSender = nic.ip();

	return pkt.send(nic,packet->hwSender,pkt.size(),ARP::ETHER_TYPE);
}

ssize_t ARP::receive(NIC &nic,Ethernet<ARP> *packet,size_t) {
	const ARP &arp = packet->payload;
	switch(be16tocpu(arp.cmd)) {
		case CMD_REQUEST:
			return handleRequest(nic,&arp);

		case CMD_REPLY:
			std::cout << "Got MAC " << arp.hwSender << " for IP " << arp.ipSender << std::endl;
			_cache[arp.ipSender] = arp.hwSender;
			sendPending(nic);
			return 0;
	}
	return 0;
}
