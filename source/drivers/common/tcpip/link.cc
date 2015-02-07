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

#include <sys/common.h>
#include <sys/messages.h>
#include <stdio.h>
#include <stdlib.h>

#include "proto/ethernet.h"
#include "common.h"

#define PRINT_PACKETS	0

#if PRINT_PACKETS
#	define PRINT(expr)	std::cout << expr << std::endl
#else
#	define PRINT(...)
#endif

Link::~Link() {
	destroybuf(_buffer,_bufname);
	Route::removeAll(shared_from_this());
}

ssize_t Link::read(void *buffer,size_t size) {
	ssize_t res = ::read(fd(),buffer,size);
	if(res > 0) {
		PRINT("Received packet of " << res << " bytes:\n"
			<< *reinterpret_cast<Ethernet<>*>(buffer));
		_rxpkts++;
		_rxbytes += res;
	}
	return res;
}

ssize_t Link::write(const void *buffer,size_t size) {
	ssize_t res = ::write(fd(),buffer,size);
	if(res > 0) {
		PRINT("Sent packet of " << res << " bytes:\n"
			<< *reinterpret_cast<const Ethernet<>*>(buffer));
		_txpkts++;
		_txbytes += res;
	}
	return res;
}
