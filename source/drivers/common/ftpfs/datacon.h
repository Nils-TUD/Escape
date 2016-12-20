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

#include <esc/stream/istringstream.h>
#include <esc/proto/net.h>
#include <esc/proto/socket.h>
#include <sys/common.h>

#include "ctrlcon.h"

class DataCon {
public:
	explicit DataCon(CtrlConRef &ctrlRef) : _sock(getEndpoint(ctrlRef)) {
	}

	int fd() const {
		return _sock.fd();
	}

	size_t read(void *buf,size_t size) {
		return _sock.receive(buf,size);
	}
	void write(const void *buf,size_t size) {
		_sock.send(buf,size);
	}

	int sharemem(int fd,void *mem,size_t size) {
		return _sock.sharemem(fd,mem,size);
	}

	void abort() {
		_sock.abort();
	}

private:
	static esc::Socket getEndpoint(CtrlConRef &ctrlRef) {
		const char *reply = ctrlRef.get()->execute(CtrlCon::CMD_PASV,"");
		char *brace = strchr(reply,'(');
		assert(brace);
		int parts[6];
		esc::IStringStream is(brace + 1);
		for(int i = 0; i < 6; ++i) {
			is >> parts[i];
			is.get();
		}

		esc::Socket sock("/dev/socket",esc::Socket::SOCK_STREAM,esc::Socket::PROTO_TCP);
		esc::Socket::Addr addr;
		addr.family = esc::Socket::AF_INET;
		addr.d.ipv4.addr = esc::Net::IPv4Addr(parts[0],parts[1],parts[2],parts[3]).value();
		addr.d.ipv4.port = (parts[4] << 8) | parts[5];
		sock.connect(addr);
		return sock;
	}

	esc::Socket _sock;
};
