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

#include <sys/common.h>
#include <sys/endian.h>

#include "../common.h"
#include "../link.h"
#include "arp.h"

class ICMP {
	enum {
		CMD_ECHO		= 8,
		CMD_ECHO_REPLY	= 0,
	};

	static const size_t MAX_ECHO_PAYLOAD_SIZE	= 4096;

public:
	enum {
		IP_PROTO     	= 1
	};

	size_t size() const {
		return sizeof(ICMP);
	}

	static ssize_t sendEcho(const esc::Net::IPv4Addr &ip,const void *payload,size_t nbytes,
			uint16_t id,uint16_t seq) {
		return send(ip,payload,nbytes,0,CMD_ECHO,id,seq);
	}
	static ssize_t receive(const std::shared_ptr<Link> &link,const Packet &packet);

private:
	static ssize_t send(const esc::Net::IPv4Addr &ip,const void *payload,size_t nbytes,
		uint8_t code,uint8_t type,uint16_t id,uint16_t seq);
	static ssize_t handleEcho(const Ethernet<IPv4<ICMP>> *packet,size_t sz);

public:
	// header
	uint8_t type;
	uint8_t code;
	uint16_t checksum;

	// data (for echo and echo reply)
	uint16_t identifier;
	uint16_t sequence;
} A_PACKED;

static inline std::ostream &operator<<(std::ostream &os,const ICMP &p) {
	os << "ICMP payload:\n";
	os << "  type          = " << p.type << "\n";
	os << "  code          = " << p.code << "\n";
	os << "  identifier    = " << be16tocpu(p.identifier) << "\n";
	os << "  sequence      = " << be16tocpu(p.sequence) << "\n";
	return os;
}
