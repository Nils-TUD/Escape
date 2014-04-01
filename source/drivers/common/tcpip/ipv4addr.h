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
#include <string.h>
#include <ostream>
#include <iomanip>

class IPv4Addr;
static std::istream &operator>>(std::istream &is,IPv4Addr &a);

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

static inline bool operator==(const IPv4Addr &a,const IPv4Addr &b) {
	return a.bytes()[0] == b.bytes()[0] &&
		a.bytes()[1] == b.bytes()[1] &&
		a.bytes()[2] == b.bytes()[2] &&
		a.bytes()[3] == b.bytes()[3];
}
static inline bool operator!=(const IPv4Addr &a,const IPv4Addr &b) {
	return !operator==(a,b);
}
static inline bool operator<(const IPv4Addr &a,const IPv4Addr &b) {
	return a.value() < b.value();
}
static inline bool operator>=(const IPv4Addr &a,const IPv4Addr &b) {
	return !operator<(a,b);
}
static inline bool operator>(const IPv4Addr &a,const IPv4Addr &b) {
	return !operator<(b,a);
}
static inline bool operator<=(const IPv4Addr &a,const IPv4Addr &b) {
	return !operator>(a,b);
}

static inline std::istream &operator>>(std::istream &is,IPv4Addr &a) {
	char tmp[5];
	for(int i = 0; i < 4; ++i) {
		// getline stops if n-1 chars have been stored. let it consume '.'/':'
		is.getline(tmp,sizeof(tmp),i == 3 ? ':' : '.');
		a._bytes[i] = atoi(tmp);
	}
	return is;
}
static inline std::ostream &operator<<(std::ostream &os,const IPv4Addr &a) {
	return os << a.bytes()[0] << "." << a.bytes()[1] << "."
			<< a.bytes()[2] << "." << a.bytes()[3];
}

inline bool IPv4Addr::isHost(const IPv4Addr &netmask) const {
	return *this != getBroadcast(netmask) && *this != getNetwork(netmask);
}
