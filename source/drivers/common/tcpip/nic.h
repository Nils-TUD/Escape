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

#include "macaddr.h"
#include "ipv4addr.h"

class NIC {
public:
	explicit NIC(const char *path,const IPv4Addr &addr)
			: _fd(open(path,IO_MSGS | IO_READ | IO_WRITE)), _mac(getMAC()), _ip(addr),
			  _subnetmask(255,255,255,0) {
		// TODO throw exception
		if(_fd < 0)
			error("open failed");
	}

	int fd() const {
		return _fd;
	}
	const MACAddr &mac() const {
		return _mac;
	}
	const IPv4Addr &ip() const {
		return _ip;
	}
	const IPv4Addr &subnetMask() const {
		return _subnetmask;
	}

	ssize_t read(void *buffer,size_t size) {
		return ::read(_fd,buffer,size);
	}
	ssize_t write(const void *buffer,size_t size) {
		return ::write(_fd,buffer,size);
	}

private:
	MACAddr getMAC() {
		sArgsMsg msg;
		if(send(_fd,MSG_NIC_GETMAC,NULL,0) < 0)
			error("Unable to send getmac-message");
		if(receive(_fd,NULL,&msg,sizeof(msg)) < 0)
			error("Unable to receive getmac-message");
		return MACAddr(msg.arg1,msg.arg2,msg.arg3,msg.arg4,msg.arg5,msg.arg6);
	}

	int _fd;
	MACAddr _mac;
	IPv4Addr _ip;
	IPv4Addr _subnetmask;
};
