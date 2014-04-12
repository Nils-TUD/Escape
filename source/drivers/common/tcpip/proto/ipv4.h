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

#include "../common.h"
#include "../link.h"
#include "../route.h"
#include "arp.h"
#include "icmp.h"
#include "udp.h"

template<class T>
class IPv4 {
	enum {
		MORE_FRAGMENTS	= 0x2000,
		DONT_FRAGMENT	= 0x4000,
	};

public:
	enum {
		ETHER_TYPE     = 0x0800
	};

	size_t size() const {
		return sizeof(IPv4) - sizeof(payload) + payload.size();
	}

	static ssize_t send(Ethernet<IPv4<T>> *pkt,size_t sz,
			const ipc::Net::IPv4Addr &ip,uint8_t protocol) {
		const Route *route = Route::find(ip);
		if(!route)
			return -ENETUNREACH;
		return sendOver(route,pkt,sz,ip,protocol);
	}

	static ssize_t sendOver(const Route *route,Ethernet<IPv4<T>> *pkt,size_t sz,
			const ipc::Net::IPv4Addr &ip,uint8_t protocol) {
		IPv4<T> &h = pkt->payload;
		h.versionSize = (4 << 4) | 5;
		h.typeOfService = 0;
		h.packetSize = cputobe16(sz - ETHER_HEAD_SIZE);
		h.packetId = 0;	// TODO ??
		h.fragOffset = cputobe16(DONT_FRAGMENT);
		h.timeToLive = 64;
		h.protocol = protocol;
		h.src = route->link->ip();
		h.dst = ip;
		h.checksum = 0;
		h.checksum = genChecksum(reinterpret_cast<uint16_t*>(&h),sizeof(IPv4) - sizeof(h.payload));

		if(route->flags & ipc::Net::FL_USE_GW)
			return ARP::send(*route->link,pkt,sz,route->gateway,route->netmask,ETHER_TYPE);
		return ARP::send(*route->link,pkt,sz,ip,route->netmask,ETHER_TYPE);
	}

	static ssize_t receive(Link &link,Ethernet<IPv4> *packet,size_t sz) {
		switch(packet->payload.protocol) {
			case ICMP::IP_PROTO: {
				Ethernet<IPv4<ICMP>> *icmpPkt = reinterpret_cast<Ethernet<IPv4<ICMP>>*>(packet);
				// TODO if(sz >= icmpPkt->size())
				return ICMP::receive(link,icmpPkt,sz);
			}
			break;

			case UDP::IP_PROTO: {
				Ethernet<IPv4<UDP>> *udpPkt = reinterpret_cast<Ethernet<IPv4<UDP>>*>(packet);
				// TODO if(sz >= udpPkt->size())
				return UDP::receive(link,udpPkt,sz);
			}
			break;
		}
		return -ENOTSUP;
	}

	static uint16_t genChecksum(const uint16_t *data,uint16_t length) {
		uint32_t sum = 0;
		for(; length > 1; length -= 2) {
			sum += *data++;
			if(sum & 0x80000000)
				sum = (sum & 0xFFFF) + (sum >> 16);
		}
		if(length)
			sum += *data & 0xFF;

		while(sum >> 16)
			sum = (sum & 0xFFFF) + (sum >> 16);
		return ~sum;
	}

	uint8_t versionSize;
	uint8_t typeOfService;
	uint16_t packetSize;
	uint16_t packetId;
	uint16_t fragOffset;
	uint8_t timeToLive;
	uint8_t protocol;
	uint16_t checksum;
	ipc::Net::IPv4Addr src;
	ipc::Net::IPv4Addr dst;
	T payload;
} A_PACKED;

static inline std::ostream &operator<<(std::ostream &os,const IPv4<> &p) {
	os << "  IPv4 payload:\n";
	os << "  typeOfService = " << p.typeOfService << "\n";
	os << "  packetSize = " << be16tocpu(p.packetSize) << "\n";
	os << "  packetId = " << be16tocpu(p.packetId) << "\n";
	os << "  fragOffset = " << be16tocpu(p.fragOffset) << "\n";
	os << "  timeToLive = " << p.timeToLive << "\n";
	os << "  protocol = " << std::hex << std::showbase << p.protocol << std::noshowbase << std::dec << "\n";
	os << "  src = " << p.src << "\n";
	os << "  dst = " << p.dst << "\n";
	switch(p.protocol) {
		case ICMP::IP_PROTO: {
			const ICMP *icmp = reinterpret_cast<const ICMP*>(&p.payload);
			return os << *icmp;
		}
		case UDP::IP_PROTO: {
			const UDP *udp = reinterpret_cast<const UDP*>(&p.payload);
			return os << *udp;
		}
	}
	return os << "Unknown payload\n";
}
