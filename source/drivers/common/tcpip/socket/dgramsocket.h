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

#include "../common.h"
#include "../portmng.h"
#include "socket.h"

class DGramSocket : public Socket {
public:
	explicit DGramSocket(int f,int proto) : Socket(f,proto), _localIp(), _localPort() {
		if(proto != ipc::Socket::PROTO_UDP)
			VTHROWE("Protocol " << proto << " is not supported by datagram socket",-ENOTSUP);
	}
	virtual ~DGramSocket();

	virtual int bind(const ipc::Socket::Addr *sa);
	virtual ssize_t sendto(const ipc::Socket::Addr *sa,const void *buffer,size_t size);
	virtual ssize_t recvfrom(bool needsSockAddr,void *buffer,size_t size);

private:
	ipc::Net::IPv4Addr _localIp;
	ipc::port_t _localPort;
	static PortMng<PRIVATE_PORTS_CNT> _ports;
};
