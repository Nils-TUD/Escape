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
#include <algorithm>
#include <vector>

#include "../socket/rawethersock.h"
#include "../common.h"
#include "../link.h"
#include "../packet.h"
#include "arp.h"
#include "ipv4.h"

static esc::OStream &operator<<(esc::OStream &os,const Ethernet<> &p);

template<class T>
class Ethernet {
public:
	size_t size() const {
		return sizeof(dst) + sizeof(src) + sizeof(type) + payload.size();
	}

	static ssize_t send(const std::shared_ptr<Link> &link,const esc::NIC::MAC &dest,Ethernet<T> *pkt,
			size_t sz,uint16_t _type) {
		pkt->src = link->mac();
		pkt->dst = dest;
		pkt->type = cputobe16(_type);
		return link->write(pkt,sz);
	}

	static ssize_t receive(const std::shared_ptr<Link> &link,const Packet &packet) {
		const Ethernet<> *epkt = packet.data<const Ethernet<>*>();

		// give all raw ethernet socket the received packet
		for(auto it = RawEtherSocket::sockets.begin(); it != RawEtherSocket::sockets.end(); ++it) {
			if((*it)->protocol() == esc::Socket::PROTO_ANY || (*it)->protocol() == epkt->type)
				(*it)->push(esc::Socket::Addr(),packet);
		}

		switch(be16tocpu(epkt->type)) {
			case ARP::ETHER_TYPE:
				if(packet.size() >= Ethernet<ARP>().size())
					return ARP::receive(link,packet);
				break;

			case IPv4<>::ETHER_TYPE:
				if(packet.size() >= Ethernet<IPv4<>>().size())
					return IPv4<>::receive(link,packet);
				break;
		}
		return -ENOTSUP;
	}

	esc::NIC::MAC dst;
	esc::NIC::MAC src;
	uint16_t type;
	T payload;
} A_PACKED;

static inline esc::OStream &operator<<(esc::OStream &os,const Ethernet<> &p) {
	os << "Ethernet packet:\n";
	os << "  dst        = " << p.dst << "\n";
	os << "  src        = " << p.src << "\n";
	os << "  type       = " << esc::fmt(be16tocpu(p.type),"0#x",4) << "\n";
	switch(be16tocpu(p.type)) {
		case ARP::ETHER_TYPE: {
			const ARP *arp = reinterpret_cast<const ARP*>(&p.payload);
			return os << *arp;
		}
		case IPv4<>::ETHER_TYPE: {
			const IPv4<> *ipv4 = reinterpret_cast<const IPv4<>*>(&p.payload);
			return os << *ipv4;
		}
	}
	return os << "Unknown payload\n";
}
