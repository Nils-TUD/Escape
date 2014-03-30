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
#include <ostream>

#include "common.h"
#include "macaddr.h"
#include "arp.h"
#include "ipv4.h"
#include "nic.h"

template<class T>
class Ethernet {
public:
	size_t size() const {
		return sizeof(dst) + sizeof(src) + sizeof(type) + payload.size();
	}

	ssize_t send(NIC &nic,const MACAddr &dest,size_t sz,uint16_t _type) {
		src = nic.mac();
		dst = dest;
		type = cputobe16(_type);
		return nic.write(this,sz);
	}

	static ssize_t receive(NIC &nic,Ethernet *packet,size_t sz) {
		switch(be16tocpu(packet->type)) {
			case ARP::ETHER_TYPE: {
				Ethernet<ARP> *arpPkt = reinterpret_cast<Ethernet<ARP>*>(packet);
				if(sz >= arpPkt->size())
					return ARP::receive(nic,arpPkt,sz);
			}
			break;

			case IPv4<>::ETHER_TYPE: {
				Ethernet<IPv4<>> *ipPkt = reinterpret_cast<Ethernet<IPv4<>>*>(packet);
				if(sz >= ipPkt->size())
					return IPv4<>::receive(nic,ipPkt,sz);
			}
			break;
		}
		return -EINVAL;
	}

	MACAddr dst;
	MACAddr src;
	uint16_t type;
	T payload;
} A_PACKED;

static inline std::ostream &operator<<(std::ostream &os,const Ethernet<> &p) {
	os << "Ethernet packet:\n";
	os << "  dst = " << p.dst << "\n";
	os << "  src = " << p.src << "\n";
	os << "  type = " << std::hex << std::setw(4) << std::showbase
					  << be16tocpu(p.type) << std::noshowbase << std::dec << "\n";
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
