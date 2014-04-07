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
#include <esc/messages.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "ipv4addr.h"

class NICDevice : public ipc::NIC {
public:
	explicit NICDevice(const char *path,const IPv4Addr &addr)
		: ipc::NIC(path,IO_MSGS | IO_READ | IO_WRITE), _mac(getMAC()), _ip(addr),
		  _subnetmask(255,255,255,0) {
	}

	const ipc::NIC::MAC &mac() const {
		return _mac;
	}
	const IPv4Addr &ip() const {
		return _ip;
	}
	const IPv4Addr &subnetMask() const {
		return _subnetmask;
	}

	ssize_t read(void *buffer,size_t size) {
		return ::read(fd(),buffer,size);
	}
	ssize_t write(const void *buffer,size_t size) {
		return ::write(fd(),buffer,size);
	}

private:
	ipc::NIC::MAC _mac;
	IPv4Addr _ip;
	IPv4Addr _subnetmask;
};
