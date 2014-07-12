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
#include <memory>
#include <stdio.h>

/**
 * This class is intended to be used with shared_ptr in order to keep this data once and have
 * multiple references to it.
 */
struct PacketData {
	explicit PacketData(uint8_t *_data,size_t _size) : data(_data), size(_size) {
	}
	PacketData(const PacketData&) = delete;
	PacketData &operator=(const PacketData&) = delete;
	~PacketData() {
		delete[] data;
	}

	uint8_t *data;
	size_t size;
};

/**
 * The idea here is to prevent as many copies of packets as possible. To do so, we first read a
 * packet into a existing buffer (in tcpip.cc) and then create an instance of this class for the
 * packet. This one gets passed around. If a socket can directly receive the data, we directly
 * copy it to the client. Only if the client is not ready yet, we create a copy via the copy()
 * method and store it. If this happens the first time, we actually copy the data since otherwise
 * we couldn't reuse our original buffer. If we do that another time, we just copy the shared_ptr.
 */
class Packet {
public:
	explicit Packet(uint8_t *d,size_t sz) : _data(d), _size(sz), _shptr() {
	}

	template<typename T>
	T data() const {
		return reinterpret_cast<T>(_data);
	}
	size_t size() const {
		return _size;
	}

	std::shared_ptr<PacketData> copy() const {
		if(!_shptr) {
			uint8_t *d = new uint8_t[_size];
			memcpy(d,_data,_size);
			_shptr = std::shared_ptr<PacketData>(new PacketData(d,_size));
		}
		return _shptr;
	}

private:
	uint8_t *_data;
	size_t _size;
	mutable std::shared_ptr<PacketData> _shptr;
};
