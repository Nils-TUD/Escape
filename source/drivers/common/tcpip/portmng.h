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
#include <ipc/proto/socket.h>
#include <bitset>

template<size_t N>
class PortMng {
public:
	explicit PortMng(ipc::port_t base) : _base(base), _firstfree(0), _ports() {
	}

	ipc::port_t allocate() {
		for(ipc::port_t p = _firstfree; p < N; ++p) {
			if(!_ports[p]) {
				_ports[p] = true;
				_firstfree = p + 1;
				return _base + p;
			}
		}
		return 0;
	}
	void release(ipc::port_t port) {
		assert(port >= _base && port < _base + N);
		_ports[port - _base] = false;
		_firstfree = port - _base;
	}

private:
	ipc::port_t _base;
	ipc::port_t _firstfree;
	std::bitset<N> _ports;
};
