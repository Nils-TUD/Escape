/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <ipc/proto/file.h>
#include <ipc/ipcstream.h>
#include <istream>
#include <ostream>
#include <vthrow.h>

namespace ipc {

typedef uint16_t port_t;

class Socket {
public:
	enum Domain {
		AF_INET
	};
	enum Type {
		SOCK_STREAM,
		SOCK_DGRAM,
		SOCK_RAW_ETHER,
		SOCK_RAW_IP,
	};
	enum Protocol {
		PROTO_ANY	= 0,
		PROTO_IP	= 0x0800,
		PROTO_UDP	= 17,
		PROTO_TCP	= 6,
		PROTO_ICMP	= 1,
	};

	struct Addr {
	 	friend std::ostream &operator<<(std::ostream &os,const Addr &a);

		Domain family;
		union {
			char data[14];
			struct {
				port_t port;
				uint32_t addr;
			} ipv4;
		} d;
	};

	/**
	 * Creates a socket of given type.
	 *
	 * @param path the path to the device
	 * @param type the socket type
	 * @param proto the protocol
	 * @throws if the operation failed
	 */
	explicit Socket(const char *path,Type type,Protocol proto)
		: _is(buildPath(path,type,proto).c_str()) {
	}

	/**
	 * Binds the socket to given address. This assigns a name to the socket, so that others can
	 * send data to it and you can receive it via recvfrom().
	 *
	 * @param addr the address to bind to (might be only the port or port and address)
	 * @throws if the operation failed
	 */
	void bind(const Addr &addr) {
		int res;
		_is << addr << SendReceive(MSG_SOCK_BIND) >> res;
		if(res < 0)
			VTHROWE("bind(" << addr << ")",res);
	}

	/**
	 * Sends <data> to <addr>.
	 *
	 * @param addr the address to send to
	 * @param data the data to send
	 * @param size the size of the data
	 * @throws if the operation failed
	 */
	void sendto(const Addr &addr,const void *data,size_t size) {
		FileWrite::Request req(0,size,-1);
		FileWrite::Response resp;
		_is << req << addr << Send(MSG_SOCK_SENDTO) << SendData(MSG_SOCK_SENDTO,data,size);
		_is >> Receive() >> resp;
		if(resp.res < 0)
			VTHROWE("sendto(" << addr << ", " << size << ")",resp.res);
	}

	/**
	 * Receives data into <data> and puts the sender-information in <addr>.
	 *
	 * @param addr will contain the information about the sender
	 * @param data the buffer to write to
	 * @param size the size of the buffer
	 * @return the number of received bytes
	 * @throws if the operation failed
	 */
	size_t recvfrom(Addr &addr,void *data,size_t size) {
		FileRead::Request req(0,size,-1);
		FileWrite::Response resp;
		_is << req << SendReceive(MSG_SOCK_RECVFROM,false) >> resp >> addr;
		if(resp.res < 0)
			VTHROWE("recvfrom(" << size << ")",resp.res);
		_is >> ReceiveData(data,size,false);
		return resp.res;
	}

private:
	static std::string buildPath(const char *path,Type type,Protocol proto) {
		std::ostringstream os;
		os << path << "/" << type << " " << proto;
		return os.str();
	}

	IPCStream _is;
};

inline std::ostream &operator<<(std::ostream &os,const Socket::Addr &a) {
	return os << "Addr[family=" << a.family << ", addr=0x" << std::hex << a.d.ipv4.addr << std::dec
			  << ", port=" << a.d.ipv4.port << "]";
}

}
