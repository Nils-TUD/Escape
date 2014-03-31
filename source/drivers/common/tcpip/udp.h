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

#include "common.h"
#include "ipv4addr.h"
#include "nic.h"
#include "socket.h"

class UDPSocket : public Socket {
public:
	virtual int bind(const IPv4Addr &addr,uint16_t port);

	virtual ssize_t read(void *buffer,size_t size);
	virtual ssize_t write(const void *buffer,size_t size);

private:
	IPv4Addr _dstIp;
	uint16_t _dstPort;
	uint16_t _srcPort;
};

class UDP {
public:
	enum {
		IP_PROTO     	= 17
	};

	size_t size() const {
		return sizeof(UDP);
	}

	static ssize_t send(const IPv4Addr &ip,uint16_t srcp,uint16_t dstp,const void *data,size_t nbytes);
	static ssize_t receive(NIC &,Ethernet<IPv4<UDP>> *,size_t) {
		return -ENOTSUP;
	}

private:
	static uint16_t genChecksum(const IPv4Addr &src,const IPv4Addr &dst,const uint16_t *header,size_t sz);

public:
    uint16_t srcPort;
    uint16_t dstPort;
    uint16_t dataSize;
    uint16_t checksum;
} A_PACKED;

static inline std::ostream &operator<<(std::ostream &os,const UDP &p) {
	os << "  UDP payload:\n";
	os << "  srcPort = " << be16tocpu(p.srcPort) << "\n";
	os << "  dstPort = " << be16tocpu(p.dstPort) << "\n";
	os << "  dataSize = " << be16tocpu(p.dataSize) << "\n";
	return os;
}
