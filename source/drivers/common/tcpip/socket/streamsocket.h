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
#include <stdlib.h>

#include "../common.h"
#include "../portmng.h"
#include "../timeouts.h"
#include "../circularbuf.h"
#include "socket.h"

class TCP;

class StreamSocket : public Socket {
public:
	static const size_t SEND_BUF_SIZE	= 32 * 1024;
	static const size_t RECV_BUF_SIZE	= 32 * 1024;
	static const size_t FORCE_PSH_PERC	= 50;
	static const size_t DEF_MSS			= 536;

	enum State {
		STATE_CLOSED,
		STATE_SYN_SENT,
		STATE_SYN_RECEIVED,
		STATE_ESTABLISHED,
		STATE_FIN_WAIT_1,
		STATE_FIN_WAIT_2,
		STATE_CLOSE_WAIT,
		STATE_CLOSING,
		STATE_LAST_ACK,
		STATE_TIME_WAIT,
		STATE_LISTEN,
	};

	struct OptionHeader {
		uint8_t kind;
		uint8_t length;
	} A_PACKED;

	struct MSSOption {
		uint8_t kind;
		uint8_t length;
		uint16_t mss;
	} A_PACKED;

	struct CtrlPacket {
		uint8_t flags;
		MSSOption option;
		size_t optSize;
		uint timeout;
	};
	struct SynPacket {
		uint16_t mss;
		ipc::Socket::Addr src;
		uint16_t winSize;
	};

	enum {
		OPTION_MSS	= 0x2,
	};

	explicit StreamSocket(int f,int proto)
			: Socket(f,proto), _timeoutId(Timeouts::allocateId()), _localPort(), _remoteAddr(),
			  _mss(DEF_MSS), _state(STATE_CLOSED), _ctrlpkt(), _txCircle(), _rxCircle(), _push() {
		if(proto != ipc::Socket::PROTO_TCP)
			VTHROWE("Protocol " << proto << " is not supported by stream socket",-ENOTSUP);

		_rxCircle.init(0,SEND_BUF_SIZE);
		_txCircle.init((rand() << 16) | rand(),RECV_BUF_SIZE);
	}
	virtual ~StreamSocket();

	virtual int connect(const ipc::Socket::Addr *sa,msgid_t mid);
	virtual int bind(const ipc::Socket::Addr *sa);
	virtual int listen();
	virtual int accept(msgid_t mid,int nfd,ipc::ClientDevice<Socket> *dev);
	virtual ssize_t sendto(msgid_t mid,const ipc::Socket::Addr *sa,const void *buffer,size_t size);
	virtual ssize_t recvfrom(msgid_t mid,bool needsSockAddr,void *buffer,size_t size);
	virtual void push(const ipc::Socket::Addr &sa,const Packet &pkt,size_t offset);
	virtual void disconnect();

	ipc::port_t localPort() const {
		return _localPort;
	}
	ipc::Net::IPv4Addr remoteIP() const {
		return ipc::Net::IPv4Addr(_remoteAddr.d.ipv4.addr);
	}
	ipc::port_t remotePort() const {
		return _remoteAddr.d.ipv4.port;
	}
	const char *state() const {
		return stateName(_state);
	}

private:
	void state(State st);
	static uint16_t parseMSS(const TCP *tcp);

	bool closing() const {
		return _state == STATE_CLOSED || _state == STATE_CLOSING || _state == STATE_CLOSE_WAIT ||
			_state == STATE_FIN_WAIT_1 || _state == STATE_FIN_WAIT_2 || _state == STATE_LAST_ACK ||
			_state == STATE_TIME_WAIT;
	}
	bool synchronized() const {
		return _state != STATE_CLOSED && _state != STATE_SYN_RECEIVED &&
			_state != STATE_SYN_SENT && _state != STATE_LISTEN;
	}
	bool shouldPush() const {
		return closing() || (_rxCircle.hasData() && (_push || pushForced()));
	}
	bool pushForced() const {
		size_t cap = _rxCircle.capacity();
		size_t left = _rxCircle.windowSize();
		return left < (cap * FORCE_PSH_PERC) / 100;
	}

	const char *stateName(State st) const;
	ssize_t sendCtrlPkt(uint8_t flags,MSSOption *opt = NULL,bool forceACK = false);
	void sendData();
	void timeout();

	int forkSocket(int nfd,msgid_t mid,ipc::ClientDevice<Socket> *dev,SynPacket &syn,
		CircularBuf::seq_type seqNo);
	void replyRead(msgid_t mid,bool needsSrc,void *buffer,size_t size);
	template<typename T>
	void replyPending(T result) {
		if(_pending.count > 0) {
			ulong buffer[1];
			ipc::IPCStream is(fd(),buffer,sizeof(buffer),_pending.mid);
			is << ipc::ReplyData(&result,sizeof(T));
			_pending.count = 0;
		}
	}

	/* our id for programming timeouts */
	int _timeoutId;

	/* connection information */
	ipc::port_t _localPort;
	ipc::Socket::Addr _remoteAddr;
	size_t _mss;
	size_t _remoteWinSize;

	/* our state */
	State _state;

	/* for connection setup and tear down */
	CtrlPacket _ctrlpkt;

	/* circular buffers for sending and receiving */
	CircularBuf _txCircle;
	CircularBuf _rxCircle;
	bool _push;

	static PortMng<PRIVATE_PORTS_CNT> _ports;
};
