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
	explicit Socket(int f) : _close(true), _is(f) {
	}

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
		: _close(false), _is(buildPath(path,type,proto).c_str(),IO_READ | IO_WRITE | IO_MSGS) {
	}

	/**
	 * No copying; moving only
	 */
	Socket(const Socket&) = delete;
	Socket &operator=(const Socket&) = delete;
	Socket(Socket &&s) : _is(std::move(s._is)) {
	}

	/**
	 * Closes the socket
	 */
	~Socket() {
		if(_close)
			::close(_is.fd());
	}

	/**
	 * @return the file-descriptor for this socket
	 */
	int fd() const {
		return _is.fd();
	}

	/**
	 * Connects to the given remote endpoint.
	 *
	 * @param addr the address to connect to
	 * @throws if the operation failed
	 */
	void connect(const Addr &addr) {
		int res;
		_is << addr << SendReceive(MSG_SOCK_CONNECT) >> res;
		if(res < 0)
			VTHROWE("connect(" << addr << ")",res);
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
	 * Puts this socket into the listen-state in order to accept incoming connections.
	 *
	 * @throws if the operation failed
	 */
	void listen() {
		int res;
		_is << SendReceive(MSG_SOCK_LISTEN) >> res;
		if(res < 0)
			VTHROWE("listen()",res);
	}

	/**
	 * Accepts an incoming connection. To do so, this socket has to be in the listen-state. If no
	 * incoming connection has been seen yet, this call blocks. If there is any, it creates a new
	 * socket for that connection and returns it.
	 *
	 * @return the socket for the accepted connection
	 * @throws if the operation failed
	 */
	Socket accept() {
		int nfd = creatsibl(_is.fd(),0);
		if(nfd < 0)
			VTHROWE("accept()",nfd);
		return Socket(nfd);
	}

	/**
	 * Sends <data> over this socket. This does only work if we already have a connection.
	 *
	 * @param data the data to send
	 * @param size the size of the data
	 * @throws if the operation failed
	 */
	void send(const void *data,size_t size) {
		ssize_t res = write(_is.fd(),data,size);
		if(res < 0)
			VTHROWE("send(" << size << ")",res);
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
		_is << req << addr << Send(MSG_SOCK_SENDTO) << SendData(data,size,MSG_SOCK_SENDTO);
		_is >> Receive() >> resp;
		if(resp.res < 0)
			VTHROWE("sendto(" << addr << ", " << size << ")",resp.res);
	}

	/**
	 * Receives <data> over this socket. This does only work if we already have a connection.
	 *
	 * @param data the buffer to write to
	 * @param size the size of the buffer
	 * @throws if the operation failed
	 */
	size_t receive(void *data,size_t size) {
		ssize_t res = read(_is.fd(),data,size);
		if(res < 0)
			VTHROWE("receive(" << size << ")",res);
		return res;
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
		try {
			_is << req << SendReceive(MSG_SOCK_RECVFROM,false) >> resp >> addr;
		}
		catch(const std::default_error &e) {
			if(e.error() == -EINTR && cancel(_is.fd(),_is.msgid()) == 1)
				_is >> Receive() >> resp >> addr;
			else
				throw;
		}
		if(resp.res < 0)
			VTHROWE("recvfrom(" << size << ")",resp.res);
		if(resp.res > 0)
			_is >> ReceiveData(data,size,false);
		return resp.res;
	}

private:
	static std::string buildPath(const char *path,Type type,Protocol proto) {
		std::ostringstream os;
		os << path << "/" << type << " " << proto;
		return os.str();
	}

	bool _close;
	IPCStream _is;
};

inline std::ostream &operator<<(std::ostream &os,const Socket::Addr &a) {
	return os << "Addr[family=" << a.family << ", addr=0x" << std::hex << a.d.ipv4.addr << std::dec
			  << ", port=" << a.d.ipv4.port << "]";
}

}
