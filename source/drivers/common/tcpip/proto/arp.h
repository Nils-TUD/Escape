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
#include <map>

#include "../common.h"
#include "../link.h"
#include "../packet.h"

class ARP {
	enum {
		HW_ADDR_ETHER	= 1,
	};

	enum {
		CMD_REQUEST		= 1,
		CMD_REPLY		= 2,
	};

	struct PendingPacket {
		esc::Net::IPv4Addr dest;
		Ethernet<> *pkt;
		uint16_t type;
		size_t size;
	};

	typedef std::vector<PendingPacket> pending_type;
	typedef std::map<esc::Net::IPv4Addr,esc::NIC::MAC> cache_type;

public:
	enum {
		ETHER_TYPE		= 0x0806,
	};

	size_t size() const {
		return sizeof(ARP);
	}

	static ssize_t send(const std::shared_ptr<Link> &link,Ethernet<> *packet,size_t size,
			const esc::Net::IPv4Addr &ip,const esc::Net::IPv4Addr &nm,uint16_t type);
	static ssize_t receive(const std::shared_ptr<Link> &link,const Packet &packet);

	static int remove(const esc::Net::IPv4Addr &ip) {
		return _cache.erase(ip) ? 0 : -ENOTFOUND;
	}
	static ssize_t requestMAC(const std::shared_ptr<Link> &link,const esc::Net::IPv4Addr &ip);
	static void print(esc::OStream &os);

private:
	static int createPending(const void *packet,size_t size,
		const esc::Net::IPv4Addr &ip,uint16_t type);
	static void sendPending(const std::shared_ptr<Link> &link);
	static ssize_t handleRequest(const std::shared_ptr<Link> &link,const ARP *packet);

public:
	uint16_t hwAddrFmt;
	uint16_t protoAddrFmt;
	uint8_t hwAddrSize;
	uint8_t protoAddrSize;
	uint16_t cmd;

	esc::NIC::MAC hwSender;
	esc::Net::IPv4Addr ipSender;
	esc::NIC::MAC hwTarget;
	esc::Net::IPv4Addr ipTarget;

private:
	static pending_type _pending;
	static cache_type _cache;
} A_PACKED;

static inline esc::OStream &operator<<(esc::OStream &os,const ARP &p) {
	os << "ARP payload:\n";
	os << "  cmddst     = " << be16tocpu(p.cmd) << "\n";
	os << "  hwSender   = " << p.hwSender << "\n";
	os << "  ipSender   = " << p.ipSender << "\n";
	os << "  hwTarget   = " << p.hwTarget << "\n";
	os << "  ipTarget   = " << p.ipTarget << "\n";
	return os;
}
