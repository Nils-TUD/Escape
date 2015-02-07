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

#include "../socket/rawipsock.h"
#include "../common.h"
#include "../link.h"
#include "../route.h"
#include "arp.h"
#include "icmp.h"
#include "tcp.h"
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
			const esc::Net::IPv4Addr &ip,uint8_t protocol) {
		Route route = Route::find(ip);
		if(!route.valid())
			return -ENETUNREACH;
		return sendOver(route,pkt,sz,ip,protocol);
	}

	static ssize_t sendOver(const Route &route,Ethernet<IPv4<T>> *pkt,size_t sz,
			const esc::Net::IPv4Addr &ip,uint8_t protocol) {
		IPv4<T> &h = pkt->payload;
		h.versionSize = (4 << 4) | 5;
		h.typeOfServ = 0;
		h.packetSize = cputobe16(sz - ETHER_HEAD_SIZE);
		h.packetId = 0;	// TODO ??
		h.fragOffset = cputobe16(DONT_FRAGMENT);
		h.timeToLive = 64;
		h.protocol = protocol;
		h.src = route.link->ip();
		h.dst = ip;
		h.checksum = 0;
		h.checksum = esc::Net::ipv4Checksum(
			reinterpret_cast<uint16_t*>(&h),sizeof(IPv4) - sizeof(h.payload));

		Ethernet<> *epkt = reinterpret_cast<Ethernet<>*>(pkt);
		if(route.flags & esc::Net::FL_USE_GW)
			return ARP::send(route.link,epkt,sz,route.gateway,route.netmask,ETHER_TYPE);
		return ARP::send(route.link,epkt,sz,ip,route.netmask,ETHER_TYPE);
	}

	static ssize_t receive(const std::shared_ptr<Link> &link,const Packet &packet) {
		const Ethernet<IPv4> *ippkt = packet.data<Ethernet<IPv4>*>();
		uint8_t proto = ippkt->payload.protocol;

		// give all raw IP socket the received packet
		for(auto it = RawIPSocket::sockets.begin(); it != RawIPSocket::sockets.end(); ++it) {
			if((*it)->protocol() == esc::Socket::PROTO_ANY || (*it)->protocol() == proto)
				(*it)->push(esc::Socket::Addr(),packet,ETHER_HEAD_SIZE);
		}

		switch(proto) {
			case ICMP::IP_PROTO:
				return ICMP::receive(link,packet);

			case UDP::IP_PROTO:
				return UDP::receive(link,packet);

			case TCP::IP_PROTO:
				return TCP::receive(link,packet);
		}
		return -ENOTSUP;
	}

	uint8_t versionSize;
	uint8_t typeOfServ;
	uint16_t packetSize;
	uint16_t packetId;
	uint16_t fragOffset;
	uint8_t timeToLive;
	uint8_t protocol;
	uint16_t checksum;
	esc::Net::IPv4Addr src;
	esc::Net::IPv4Addr dst;
	T payload;
} A_PACKED;

static inline esc::OStream &operator<<(esc::OStream &os,const IPv4<> &p) {
	os << "IPv4 payload:\n";
	os << "  typeOfServ = " << p.typeOfServ << "\n";
	os << "  packetSize = " << be16tocpu(p.packetSize) << "\n";
	os << "  packetId   = " << be16tocpu(p.packetId) << "\n";
	os << "  fragOffset = " << be16tocpu(p.fragOffset) << "\n";
	os << "  timeToLive = " << p.timeToLive << "\n";
	os << "  protocol   = " << esc::fmt(p.protocol,"#x") << "\n";
	os << "  src        = " << p.src << "\n";
	os << "  dst        = " << p.dst << "\n";
	switch(p.protocol) {
		case ICMP::IP_PROTO: {
			const ICMP *icmp = reinterpret_cast<const ICMP*>(&p.payload);
			return os << *icmp;
		}
		case UDP::IP_PROTO: {
			const UDP *udp = reinterpret_cast<const UDP*>(&p.payload);
			return os << *udp;
		}
		case TCP::IP_PROTO: {
			const TCP *tcp = reinterpret_cast<const TCP*>(&p.payload);
			return os << *tcp;
		}
	}
	return os << "Unknown payload\n";
}
