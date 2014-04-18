/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/messages.h>
#include <ipc/proto/nic.h>
#include <ipc/ipcstream.h>
#include <istream>
#include <ostream>
#include <vthrow.h>

namespace ipc {

class Net {
public:
	class IPv4Addr {
		friend std::istream &operator>>(std::istream &is,IPv4Addr &a);

	public:
		static const size_t LEN	= 4;

		explicit IPv4Addr() : _bytes() {
		}
		explicit IPv4Addr(uint32_t val) {
			_bytes[0] = val >> 24;
			_bytes[1] = (val >> 16) & 0xFF;
			_bytes[2] = (val >> 8) & 0xFF;
			_bytes[3] = val & 0xFF;
		}
		explicit IPv4Addr(uint8_t *b) {
			memcpy(_bytes,b,LEN);
		}
		explicit IPv4Addr(uint8_t a,uint8_t b,uint8_t c,uint8_t d) {
			_bytes[0] = a;
			_bytes[1] = b;
			_bytes[2] = c;
			_bytes[3] = d;
		}

		const uint8_t *bytes() const {
			return _bytes;
		}
		uint32_t value() const {
			return (_bytes[0] << 24) | (_bytes[1] << 16) | (_bytes[2] << 8) | _bytes[3];
		}
		bool sameNetwork(const IPv4Addr &ip,const IPv4Addr &netmask) const {
			return (value() & netmask.value()) == (ip.value() & netmask.value());
		}
		bool isHost(const IPv4Addr &netmask) const;
		bool isNetmask() const {
			// a netmask has only 1's beginning from the MSB and afterwards only 0's.
			// so, for the complement it's the other way around, which means, that (complement + 1)
			// has to be a power of 2.
			uint32_t complement = ~value();
			return ((complement + 1) & complement) == 0;
		}

		IPv4Addr getBroadcast(const IPv4Addr &netmask) const {
			return IPv4Addr(value() | ~netmask.value());
		}
		IPv4Addr getNetwork(const IPv4Addr &netmask) const {
			return IPv4Addr(value() & netmask.value());
		}

	private:
		uint8_t _bytes[LEN];
	} A_PACKED;

	enum Status {
		UP,
		DOWN,
		KILLED
	};
	enum RouteFlags {
		FL_USE_GW	= 0x1,
		FL_UP		= 0x2,
	};

	/**
	 * Opens the given device
	 *
	 * @param path the path to the device
	 * @throws if the operation failed
	 */
	explicit Net(const char *path) : _is(path) {
	}

	void linkAdd(const char *name,const char *device) {
		int res;
		_is << CString(name) << CString(device) << SendReceive(MSG_NET_LINK_ADD) >> res;
		if(res < 0)
			VTHROWE("linkAdd(" << name << "," << device << ")",res);
	}
	void linkRem(const char *name) {
		int res;
		_is << CString(name) << SendReceive(MSG_NET_LINK_REM) >> res;
		if(res < 0)
			VTHROWE("linkRem(" << name << ")",res);
	}
	void linkConfig(const char *link,const IPv4Addr &ip,const IPv4Addr &nm,Status status) {
		int res;
		_is << CString(link) << ip << nm << status << SendReceive(MSG_NET_LINK_CONFIG) >> res;
		if(res < 0)
			VTHROWE("linkConfig(" << link << ")",res);
	}
	NIC::MAC linkMAC(const char *link) {
		NIC::MAC mac;
		int res;
		_is << CString(link) << SendReceive(MSG_NET_LINK_MAC) >> res >> mac;
		if(res < 0)
			VTHROWE("linkMAC(" << link << ")",res);
		return mac;
	}

	void routeAdd(const char *link,const IPv4Addr &ip,const IPv4Addr &gw,const IPv4Addr &nm) {
		int res;
		_is << CString(link) << ip << gw << nm << SendReceive(MSG_NET_ROUTE_ADD) >> res;
		if(res < 0)
			VTHROWE("routeAdd(" << link << ")",res);
	}
	void routeRem(const IPv4Addr &ip) {
		int res;
		_is << ip << SendReceive(MSG_NET_ROUTE_REM) >> res;
		if(res < 0)
			VTHROWE("routeRem()",res);
	}
	IPv4Addr routeGet(const IPv4Addr &ip,char *linkname,size_t size) {
		IPv4Addr dest;
		CString link(linkname,size);
		int res;
		_is << ip << SendReceive(MSG_NET_ROUTE_GET) >> res >> dest >> link;
		if(res < 0)
			VTHROWE("routeGem()",res);
		return dest;
	}
	void routeConfig(const IPv4Addr &ip,Status status) {
		int res;
		_is << ip << status << SendReceive(MSG_NET_ROUTE_CONFIG) >> res;
		if(res < 0)
			VTHROWE("routeConfig()",res);
	}

	void arpAdd(const IPv4Addr &ip) {
		int res;
		_is << ip << SendReceive(MSG_NET_ARP_ADD) >> res;
		if(res < 0)
			VTHROWE("arpAdd()",res);
	}
	void arpRem(const IPv4Addr &ip) {
		int res;
		_is << ip << SendReceive(MSG_NET_ARP_REM) >> res;
		if(res < 0)
			VTHROWE("arpRem()",res);
	}

	static uint16_t ipv4Checksum(const uint16_t *data,uint16_t length);
	static uint16_t ipv4PayloadChecksum(const IPv4Addr &src,const IPv4Addr &dst,uint16_t protocol,
		const uint16_t *header,size_t sz);

private:
	IPCStream _is;
};

static inline bool operator==(const Net::IPv4Addr &a,const Net::IPv4Addr &b) {
	return a.bytes()[0] == b.bytes()[0] &&
		a.bytes()[1] == b.bytes()[1] &&
		a.bytes()[2] == b.bytes()[2] &&
		a.bytes()[3] == b.bytes()[3];
}
static inline bool operator!=(const Net::IPv4Addr &a,const Net::IPv4Addr &b) {
	return !operator==(a,b);
}
static inline bool operator<(const Net::IPv4Addr &a,const Net::IPv4Addr &b) {
	return a.value() < b.value();
}
static inline bool operator>=(const Net::IPv4Addr &a,const Net::IPv4Addr &b) {
	return !operator<(a,b);
}
static inline bool operator>(const Net::IPv4Addr &a,const Net::IPv4Addr &b) {
	return !operator<(b,a);
}
static inline bool operator<=(const Net::IPv4Addr &a,const Net::IPv4Addr &b) {
	return !operator>(a,b);
}

inline std::istream &operator>>(std::istream &is,Net::IPv4Addr &a) {
	for(size_t i = 0; i < Net::IPv4Addr::LEN; ++i) {
		unsigned val = 0;
		is >> val;
		if(i < Net::IPv4Addr::LEN - 1) {
			// skip '.'
			is.get();
		}
		a._bytes[i] = val;
	}
	return is;
}
static inline std::ostream &operator<<(std::ostream &os,const Net::IPv4Addr &a) {
	return os << a.bytes()[0] << "." << a.bytes()[1] << "."
			<< a.bytes()[2] << "." << a.bytes()[3];
}

inline bool Net::IPv4Addr::isHost(const Net::IPv4Addr &netmask) const {
	return *this != getBroadcast(netmask) && *this != getNetwork(netmask);
}

}
