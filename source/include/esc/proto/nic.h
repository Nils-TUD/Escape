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
#include <sys/messages.h>
#include <esc/proto/default.h>
#include <ostream>
#include <iomanip>
#include <esc/vthrow.h>

namespace esc {

/**
 * The IPC-interface for NIC devices.
 */
class NIC {
public:
	static const unsigned PCI_CLASS		= 0x02;
	static const unsigned PCI_SUBCLASS	= 0x00;

	/**
	 * Represents a MAC address
	 */
	class MAC {
		friend std::istream &operator>>(std::istream &is,NIC::MAC &a);

	public:
		static const size_t LEN	= 6;

		static MAC broadcast() {
			return MAC(0xFF,0xFF,0xFF,0xFF,0xFF,0xFF);
		}

		explicit MAC() : _bytes() {
		}
		explicit MAC(const uint8_t *b) : MAC(b[0],b[1],b[2],b[3],b[4],b[5]) {
		}
		explicit MAC(uint8_t b1,uint8_t b2,uint8_t b3,uint8_t b4,uint8_t b5,uint8_t b6) {
			_bytes[0] = b1;
			_bytes[1] = b2;
			_bytes[2] = b3;
			_bytes[3] = b4;
			_bytes[4] = b5;
			_bytes[5] = b6;
		}

		const uint8_t *bytes() const {
			return _bytes;
		}
		uint64_t value() const {
			return (uint64_t)_bytes[5] << 40 |
				(uint64_t)_bytes[4] << 32 |
				(uint64_t)_bytes[3] << 24 |
				(uint64_t)_bytes[2] << 16 |
				(uint64_t)_bytes[1] << 8 |
				(uint64_t)_bytes[0] << 0;
		}

	private:
		uint8_t _bytes[LEN];
	} A_PACKED;

	/**
	 * Opens the given device
	 *
	 * @param path the path to the device
	 * @param flags the open-flags
	 * @throws if the operation failed
	 */
	explicit NIC(const char *path,uint flags = O_MSGS) : _is(path,flags) {
	}

	/**
	 * No copying
	 */
	NIC(const NIC&) = delete;
	NIC &operator=(const NIC&) = delete;

	/**
	 * @return the file-descriptor
	 */
	int fd() const {
		return _is.fd();
	}

	/**
	 * @return the maximum transmission unit, i.e. the maximum packet size
	 * @throws if the operation failed
	 */
	ulong getMTU() {
		long res;
		_is << SendReceive(MSG_NIC_GETMTU) >> res;
		if(res < 0)
			VTHROWE("getMTU()",res);
		return res;
	}

	/**
	 * @return the MAC address of the NIC
	 * @throws if the operation failed
	 */
	MAC getMAC() {
		int res;
		MAC addr;
		_is << SendReceive(MSG_NIC_GETMAC) >> res >> addr;
		if(res < 0)
			VTHROWE("getMAC()",res);
		return addr;
	}

private:
	IPCStream _is;
};

static inline bool operator==(const NIC::MAC &a,const NIC::MAC &b) {
	return a.bytes()[0] == b.bytes()[0] &&
		a.bytes()[1] == b.bytes()[1] &&
		a.bytes()[2] == b.bytes()[2] &&
		a.bytes()[3] == b.bytes()[3] &&
		a.bytes()[4] == b.bytes()[4] &&
		a.bytes()[5] == b.bytes()[5];
}
static inline bool operator!=(const NIC::MAC &a,const NIC::MAC &b) {
	return !operator==(a,b);
}
static inline bool operator<(const NIC::MAC &a,const NIC::MAC &b) {
	uint64_t vala = ((uint64_t)a.bytes()[0] << 40) | ((uint64_t)a.bytes()[1] << 32) |
					(a.bytes()[2] << 24) | (a.bytes()[3] << 16) |
					(a.bytes()[4] <<  8) | (a.bytes()[5] <<  0);
	uint64_t valb = ((uint64_t)b.bytes()[0] << 40) | ((uint64_t)b.bytes()[1] << 32) |
					(b.bytes()[2] << 24) | (b.bytes()[3] << 16) |
					(b.bytes()[4] <<  8) | (b.bytes()[5] <<  0);
	return vala < valb;
}

static inline std::ostream &operator<<(std::ostream &os,const NIC::MAC &a) {
	return os << std::hex << std::setfill('0')
		<< std::setw(2) << a.bytes()[0] << ":"
		<< std::setw(2) << a.bytes()[1] << ":"
		<< std::setw(2) << a.bytes()[2] << ":"
		<< std::setw(2) << a.bytes()[3] << ":"
		<< std::setw(2) << a.bytes()[4] << ":"
		<< std::setw(2) << a.bytes()[5]
		<< std::setfill(' ') << std::dec;
}
inline std::istream &operator>>(std::istream &is,NIC::MAC &a) {
	is >> std::hex;
	for(size_t i = 0; i < NIC::MAC::LEN; ++i) {
		unsigned val;
		is >> val;
		if(i < NIC::MAC::LEN - 1) {
			// skip ':'
			is.get();
		}
		a._bytes[i] = val;
	}
	return is >> std::dec;
}

}
