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
#include <iostream>

#include "../proto/udp.h"
#include "dgramsocket.h"

PortMng<PRIVATE_PORTS_CNT> DGramSocket::_ports(PRIVATE_PORTS);

DGramSocket::~DGramSocket() {
	if(_localPort != 0) {
		UDP::remSocket(this,_localPort);
		if(_localPort >= PRIVATE_PORTS)
			_ports.release(_localPort);
	}
}

int DGramSocket::bind(const ipc::Socket::Addr *sa) {
	if(sa->d.ipv4.port >= PRIVATE_PORTS)
		return -EINVAL;
	if(_localPort != 0)
		return -EINVAL;

	int res = UDP::addSocket(this,sa->d.ipv4.port);
	if(res == 0)
		_localPort = sa->d.ipv4.port;
	return res;
}

ssize_t DGramSocket::sendto(msgid_t,const ipc::Socket::Addr *sa,const void *buffer,size_t size) {
	if(sa == NULL)
		return -EINVAL;

	// do we still need a local port?
	if(_localPort == 0) {
		_localPort = _ports.allocate();
		if(_localPort == 0)
			return -EAGAIN;
		UDP::addSocket(this,_localPort);
	}

	return UDP::send(ipc::Net::IPv4Addr(sa->d.ipv4.addr),_localPort,sa->d.ipv4.port,buffer,size);
}
