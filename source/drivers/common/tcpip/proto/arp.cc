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

int ARP::createPending(const void *packet,size_t size,const ipc::Net::IPv4Addr &ip,uint16_t type) {
	PendingPacket pkt;
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

void ARP::sendPending(Link &link) {
	for(auto it = _pending.begin(); it < _pending.end(); ) {
		cache_type::iterator entry = _cache.find(it->dest);
		if(entry != _cache.end()) {
			Ethernet<>::send(link,entry->second,it->pkt,it->size,it->type);
			_pending.erase(it);
			free(it->pkt);
		}
		else
			it++;
	}
}

ssize_t ARP::requestMAC(Link &link,const ipc::Net::IPv4Addr &ip) {
	Ethernet<ARP> pkt;
	ARP *arp = &pkt.payload;

	arp->hwAddrFmt = cputobe16(HW_ADDR_ETHER);
	arp->hwAddrSize = sizeof(ipc::NIC::MAC);
	arp->protoAddrFmt = cputobe16(IPv4<>::ETHER_TYPE);
	arp->protoAddrSize = ipc::Net::IPv4Addr::LEN;
	arp->cmd = cputobe16(CMD_REQUEST);

	arp->hwTarget = ipc::NIC::MAC();
	arp->ipTarget = ip;
	arp->hwSender = link.mac();
	arp->ipSender = link.ip();

	return Ethernet<ARP>::send(link,ipc::NIC::MAC::broadcast(),&pkt,pkt.size(),ARP::ETHER_TYPE);
}

ssize_t ARP::handleRequest(Link &link,const ARP *packet) {
	// not a valid host in our network?
	if(!packet->ipSender.isHost(link.subnetMask()))
		return -EINVAL;
	// TODO multicast is invalid too
	if(packet->hwSender == ipc::NIC::MAC::broadcast())
		return -EINVAL;

	// store the mapping in every case. perhaps we need it in future
	_cache[packet->ipSender] = packet->hwSender;

	// not for us?
	if(packet->ipTarget != link.ip())
		return 0;

	// reply our MAC address to sender
	Ethernet<ARP> pkt;
	ARP *arp = &pkt.payload;

	arp->hwAddrFmt = cputobe16(HW_ADDR_ETHER);
	arp->hwAddrSize = sizeof(ipc::NIC::MAC);
	arp->protoAddrFmt = cputobe16(IPv4<>::ETHER_TYPE);
	arp->protoAddrSize = ipc::Net::IPv4Addr::LEN;
	arp->cmd = cputobe16(CMD_REPLY);

	arp->hwTarget = packet->hwSender;
	arp->ipTarget = packet->ipSender;
	arp->hwSender = link.mac();
	arp->ipSender = link.ip();

	return Ethernet<ARP>::send(link,packet->hwSender,&pkt,pkt.size(),ARP::ETHER_TYPE);
}

ssize_t ARP::send(Link &link,Ethernet<> *packet,size_t size,const ipc::Net::IPv4Addr &ip,
		const ipc::Net::IPv4Addr &nm,uint16_t type) {
	ipc::NIC::MAC mac;
	if(ip == ip.getBroadcast(nm))
		mac = ipc::NIC::MAC::broadcast();
	// ARP requests for ourself don't work since we don't get our own broadcasts
	else if(ip == link.ip())
		mac = link.mac();
	else {
		cache_type::iterator it = _cache.find(ip);

		// if we don't know the MAC address yet, start an ARP request and add packet to pending list
		if(it == _cache.end()) {
			int res = createPending(packet,size,ip,type);
			if(res < 0)
				return res;
			return requestMAC(link,ip);
		}
		mac = it->second;
	}

	// otherwise just send the packet
	return Ethernet<>::send(link,mac,packet,size,type);
}

ssize_t ARP::receive(Link &link,const Packet &packet) {
	const ARP &arp = packet.data<Ethernet<ARP>*>()->payload;
	switch(be16tocpu(arp.cmd)) {
		case CMD_REQUEST:
			return handleRequest(link,&arp);

		case CMD_REPLY:
			std::cout << "Got MAC " << arp.hwSender << " for IP " << arp.ipSender << std::endl;
			_cache[arp.ipSender] = arp.hwSender;
			sendPending(link);
			return 0;
	}
	return 0;
}

void ARP::print(std::ostream &os) {
	for(auto it = _cache.begin(); it != _cache.end(); ++it)
		os << it->first << " " << it->second << "\n";
}
