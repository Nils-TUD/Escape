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
#include <map>

#include "common.h"
#include "ipv4addr.h"
#include "nic.h"

class ARP {
	enum {
		HW_ADDR_ETHER	= 1,
	};

	enum {
		CMD_REQUEST		= 1,
		CMD_REPLY		= 2,
	};

	struct Packet {
		IPv4Addr dest;
		Ethernet<> *pkt;
		uint16_t type;
		size_t size;
	};

	typedef std::vector<Packet> pending_type;
	typedef std::map<IPv4Addr,ipc::NIC::MAC> cache_type;

public:
	enum {
		ETHER_TYPE		= 0x0806,
	};

	size_t size() const {
		return sizeof(ARP);
	}

	template<class T>
	static ssize_t send(NICDevice &nic,Ethernet<T> *packet,size_t size,const IPv4Addr &ip,uint16_t type) {
		cache_type::iterator it = _cache.find(ip);

		// if we don't know the MAC address yet, start an ARP request and add packet to pending list
		if(it == _cache.end()) {
			int res = createPending(packet,size,ip,type);
			if(res < 0)
				return res;
			return requestMAC(nic,ip);
		}

		// otherwise just send the packet
		return packet->send(nic,it->second,size,type);
	}
	static ssize_t receive(NICDevice &nic,Ethernet<ARP> *packet,size_t size);

	static ssize_t requestMAC(NICDevice &nic,const IPv4Addr &ip);

private:
	static int createPending(const void *packet,size_t size,const IPv4Addr &ip,uint16_t type);
	static void sendPending(NICDevice &nic);
	static ssize_t handleRequest(NICDevice &nic,const ARP *packet);

public:
	uint16_t hwAddrFmt;
	uint16_t protoAddrFmt;
	uint8_t hwAddrSize;
	uint8_t protoAddrSize;
	uint16_t cmd;

	ipc::NIC::MAC hwSender;
	IPv4Addr ipSender;
	ipc::NIC::MAC hwTarget;
	IPv4Addr ipTarget;

private:
	static pending_type _pending;
	static cache_type _cache;
} A_PACKED;

static inline std::ostream &operator<<(std::ostream &os,const ARP &p) {
	os << "  ARP payload:\n";
	os << "  cmd = " << be16tocpu(p.cmd) << "\n";
	os << "  hwSender = " << p.hwSender << "\n";
	os << "  ipSender = " << p.ipSender << "\n";
	os << "  hwTarget = " << p.hwTarget << "\n";
	os << "  ipTarget = " << p.ipTarget << "\n";
	return os;
}
