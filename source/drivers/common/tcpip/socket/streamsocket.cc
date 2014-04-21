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

#include <esc/common.h>

#include "../common.h"
#include "../proto/ethernet.h"
#include "../proto/ipv4.h"
#include "../proto/tcp.h"
#include "streamsocket.h"

/**
 * State transitions from http://www.rfc-editor.org/rfc/rfc793.txt:
 *
 *                              +---------+ ---------\      active OPEN
 *                              |  CLOSED |            \    -----------
 *                              +---------+<---------\   \   create TCB
 *                                |     ^              \   \  snd SYN
 *                   passive OPEN |     |   CLOSE        \   \
 *                   ------------ |     | ----------       \   \
 *                    create TCB  |     | delete TCB         \   \
 *                                V     |                      \   \
 *                              +---------+            CLOSE    |    \
 *                              |  LISTEN |          ---------- |     |
 *                              +---------+          delete TCB |     |
 *                   rcv SYN      |     |     SEND              |     |
 *                  -----------   |     |    -------            |     V
 * +---------+      snd SYN,ACK  /       \   snd SYN          +---------+
 * |         |<-----------------           ------------------>|         |
 * |   SYN   |                    rcv SYN                     |   SYN   |
 * |   RCVD  |<-----------------------------------------------|   SENT  |
 * |         |                    snd ACK                     |         |
 * |         |------------------           -------------------|         |
 * +---------+   rcv ACK of SYN  \       /  rcv SYN,ACK       +---------+
 *   |           --------------   |     |   -----------
 *   |                  x         |     |     snd ACK
 *   |                            V     V
 *   |  CLOSE                   +---------+
 *   | -------                  |  ESTAB  |
 *   | snd FIN                  +---------+
 *   |                   CLOSE    |     |    rcv FIN
 *   V                  -------   |     |    -------
 * +---------+          snd FIN  /       \   snd ACK          +---------+
 * |  FIN    |<-----------------           ------------------>|  CLOSE  |
 * | WAIT-1  |------------------                              |   WAIT  |
 * +---------+          rcv FIN  \                            +---------+
 *   | rcv ACK of FIN   -------   |                            CLOSE  |
 *   | --------------   snd ACK   |                           ------- |
 *   V        x                   V                           snd FIN V
 * +---------+                  +---------+                   +---------+
 * |FINWAIT-2|                  | CLOSING |                   | LAST-ACK|
 * +---------+                  +---------+                   +---------+
 *   |                rcv ACK of FIN |                 rcv ACK of FIN |
 *   |  rcv FIN       -------------- |    Timeout=2MSL -------------- |
 *   |  -------              x       V    ------------        x       V
 *    \ snd ACK                 +---------+delete TCB         +---------+
 *     ------------------------>|TIME WAIT|------------------>| CLOSED  |
 *                              +---------+                   +---------+
 */


PortMng<PRIVATE_PORTS_CNT> StreamSocket::_ports(PRIVATE_PORTS);

StreamSocket::~StreamSocket() {
	if(_localPort != 0) {
		TCP::remSocket(this,_localPort,remotePort());
		if(_localPort >= PRIVATE_PORTS)
			_ports.release(_localPort);
	}
	Timeouts::cancel(_timeoutId);
}

void StreamSocket::state(State st) {
	PRINT_TCP(_localPort,remotePort(),"went from %s to %s",stateName(_state),stateName(st));
	_state = st;
}

int StreamSocket::connect(const ipc::Socket::Addr *sa,msgid_t mid) {
	if(_state != STATE_CLOSED)
		return -EISCONN;

	_remoteAddr = *sa;
	const Route *route = Route::find(remoteIP());
	if(!route)
		return -ENETUNREACH;

	// do we still need a local port?
	if(_localPort == 0) {
		_localPort = _ports.allocate();
		if(_localPort == 0)
			return -EAGAIN;
		TCP::addSocket(this,_localPort,remotePort());
	}

	MSSOption option;
	option.kind = OPTION_MSS;
	option.length = sizeof(option);
	option.mss = cputobe16(route->link->mtu() - IPv4<TCP>().size());

	// send SYN packet
	ssize_t res = sendCtrlPkt(TCP::FL_SYN,&option);
	if(res < 0)
		return res;

	state(STATE_SYN_SENT);
	_pending.mid = mid;
	_pending.count = 1;
	return 0;
}

int StreamSocket::bind(const ipc::Socket::Addr *sa) {
	if(_localPort != 0)
		return -EINVAL;
	if(sa->d.ipv4.port == 0 || sa->d.ipv4.port >= PRIVATE_PORTS)
		return -EINVAL;

	_localPort = sa->d.ipv4.port;
	return TCP::addSocket(this,_localPort,0);
}

int StreamSocket::listen() {
	if(_state != STATE_CLOSED)
		return -EINVAL;
	if(_localPort == 0)
		return -ENOTBOUND;

	state(STATE_LISTEN);
	return 0;
}

int StreamSocket::accept(msgid_t mid,int nfd,ipc::ClientDevice<Socket> *dev) {
	if(_state != STATE_LISTEN)
		return -EINVAL;

	SynPacket syn;
	CircularBuf::seq_type seqNo;
	if(_rxCircle.pullctrl(&syn,sizeof(syn),&seqNo))
		return forkSocket(nfd,mid,dev,syn,seqNo);

	if(_pending.count > 0)
		return -EAGAIN;
	_pending.count = 1;
	_pending.mid = mid;
	_pending.d.accept.fd = fd();
	_pending.d.accept.nfd = nfd;
	_pending.d.accept.dev = dev;
	return 0;
}

ssize_t StreamSocket::sendto(msgid_t mid,const ipc::Socket::Addr *,const void *data,size_t size) {
	if(_state != STATE_ESTABLISHED)
		return -ENOTCONN;

	CircularBuf::seq_type seqNo = _txCircle.nextSeq();
	// TODO handle requests that are larger. probably we want to increase the txCircle in this case
	if(size > _txCircle.windowSize())
		return -EINVAL;

	// push it into our txCircle
	assert(_txCircle.push(seqNo,CircularBuf::TYPE_DATA,data,size) == (ssize_t)size);
	// send it
	sendData();

	// register request; the response is sent as soon as the sent data has been ACKed
	_pending.mid = mid;
	_pending.count = size;
	_pending.d.write.seqNo = seqNo + size;
	Timeouts::program(_timeoutId,std::make_memfun(this,&StreamSocket::timeout),1000);
	return 0;
}

ssize_t StreamSocket::recvfrom(msgid_t mid,bool needsSrc,void *buffer,size_t size) {
	if(size == 0)
		return -EINVAL;
	if(_state == STATE_CLOSED || _state == STATE_SYN_SENT)
		return -ENOTCONN;

	if(shouldPush()) {
		replyRead(mid,needsSrc,buffer,size);
		return 0;
	}

	if(_pending.count > 0)
		return -EAGAIN;
	_pending.mid = mid;
	_pending.count = size;
	_pending.d.read.data = buffer;
	_pending.d.read.needsSrc = needsSrc;
	return 0;
}

void StreamSocket::disconnect() {
	switch(_state) {
		// if the socket is completely closed, we can destroy it immediately
		case STATE_CLOSED:
		// if we're not connected yet, there is no reason to wait or start the disconnect procedure
		case STATE_SYN_SENT:
		case STATE_LISTEN:
			delete this;
			return;

		case STATE_LAST_ACK:
		case STATE_FIN_WAIT_1:
		case STATE_FIN_WAIT_2:
		case STATE_CLOSING:
		case STATE_TIME_WAIT:
			// nothing to do. we wait until we're in STATE_CLOSED.
			break;

		case STATE_CLOSE_WAIT:
			sendCtrlPkt(TCP::FL_FIN | TCP::FL_ACK);
			state(STATE_LAST_ACK);
			break;

		default:
			sendCtrlPkt(TCP::FL_FIN);
			state(STATE_FIN_WAIT_1);
			break;
	}
}

void StreamSocket::timeout() {
	switch(_state) {
		case STATE_TIME_WAIT: {
			state(STATE_CLOSED);
			delete this;
		}
		break;

		case STATE_ESTABLISHED:
			if(_pending.count > 0) {
				PRINT_TCP(_localPort,remotePort(),"timeout. Resending data.");
				sendData();
			}
			break;

		default: {
			// if there is an un-ACKed control-packet, resend it
			if(_ctrlpkt.flags) {
				PRINT_TCP(_localPort,remotePort(),"timeout. Resending control-packet.");
				if(_ctrlpkt.timeout < 8000) {
					_ctrlpkt.timeout *= 2;
					ssize_t res = TCP::send(remoteIP(),_localPort,remotePort(),
						_ctrlpkt.flags,&_ctrlpkt.option,_ctrlpkt.optSize,_ctrlpkt.optSize,
						_txCircle.nextSeq(),_rxCircle.nextExp(),_rxCircle.windowSize());
					if(res < 0) {
						// TODO handle error
						printe("TCP::send");
					}
					Timeouts::program(_timeoutId,std::make_memfun(this,&StreamSocket::timeout),
						_ctrlpkt.timeout);
				}
				else {
					state(STATE_CLOSED);
					replyPending<int>(-ETIMEOUT);
				}
			}
			else
				print("Timeout for a non-control-packet during state %s!?",stateName(_state));
		}
		break;
	}
}

void StreamSocket::push(const ipc::Socket::Addr &,const Packet &pkt,size_t) {
	const Ethernet<IPv4<TCP>> *epkt = pkt.data<const Ethernet<IPv4<TCP>>*>();
	const IPv4<TCP> *ip = &epkt->payload;
	const TCP *tcp = &epkt->payload.payload;
	State oldstate = _state;

	size_t tcplen = be16tocpu(ip->packetSize) - IPv4<>().size();
	size_t dataOff = (tcp->dataOffset >> 4) * 4;
	size_t seglen = tcplen - dataOff;

  	CircularBuf::seq_type seqNo = be32tocpu(tcp->seqNumber);
	CircularBuf::seq_type ackNo = be32tocpu(tcp->ackNumber);
	_remoteWinSize = be16tocpu(tcp->windowSize);

	// validate checksum
	uint16_t checksum = ipc::Net::ipv4PayloadChecksum(ip->src,ip->dst,TCP::IP_PROTO,
		reinterpret_cast<const uint16_t*>(tcp),tcplen);
	if(checksum != 0) {
		PRINT_TCP(_localPort,remotePort(),"packet has invalid checksum (%#04x). Dropping",checksum);
		return;
	}

	// should we abort the connection?
	if(tcp->ctrlFlags & TCP::FL_RST) {
		// first check if it's valid (in SYN_SENT state the ackNo has to ACK the SYN)
		if((_state == STATE_SYN_SENT && ackNo == _txCircle.nextExp()) || _rxCircle.isInWindow(seqNo)) {
			PRINT_TCP(_localPort,remotePort(),"received valid RST");
			switch(_state) {
				case STATE_LISTEN:
					// ignore
					break;

				// this is already done in the client-socket, so just go back to closed.
				case STATE_SYN_RECEIVED:
				default:
					state(STATE_CLOSED);
					replyPending(-ECONNRESET);
					break;
			}
		}
		return;
	}

	bool ackForced = false;

	// in state SYN_SENT we don't have a initialized _rxCircle yet
	if(_state != STATE_SYN_SENT && _state != STATE_LISTEN) {
		// do we have received a non-ACK packet?
		if(((tcp->ctrlFlags & ~TCP::FL_ACK) || seglen > 0)) {
			// determine type of packet
			uint8_t type = seglen ? CircularBuf::TYPE_DATA : CircularBuf::TYPE_CTRL;
		  	const uint8_t *data = seglen ? reinterpret_cast<const uint8_t*>(tcp) + dataOff : NULL;

		  	// only accept data in established state
		  	if(type == CircularBuf::TYPE_CTRL || _state == STATE_ESTABLISHED) {
		  		if(_rxCircle.push(seqNo,type,data,seglen) < 0) {
		  			if(synchronized()) {
						PRINT_TCP(_localPort,remotePort(),"received unexpected seq %u, expected %u",
							seqNo,_rxCircle.nextExp());
		  				sendCtrlPkt(TCP::FL_ACK);
		  			}
					return;
				}
			}
		}
	}

	// handle acks
	if(tcp->ctrlFlags & TCP::FL_ACK) {
		int res = _txCircle.forget(ackNo);
		if(res < 0) {
			PRINT_TCP(_localPort,remotePort(),"received unexpected ack %u, expected %u",
				ackNo,_txCircle.nextSeq());
			if(!synchronized())
				TCP::replyReset(epkt);
			else
				ackForced = true;
		}
		else if(_pending.count > 0 && _pending.isWrite()) {
			if(ackNo == _pending.d.write.seqNo) {
				replyPending<ssize_t>(_pending.count);
				Timeouts::cancel(_timeoutId);
			}
		}
		// if this is an ACK for our last control packet, stop waiting for it
		else if(_ctrlpkt.flags != 0) {
			_ctrlpkt.flags = 0;
			Timeouts::cancel(_timeoutId);
		}
	}

	// send outstanding data
	sendData();

	// handle state changes
	switch(_state) {
		case STATE_LISTEN: {
			if(tcp->ctrlFlags == TCP::FL_SYN) {
				SynPacket syn;
				syn.mss = parseMSS(tcp);
				syn.winSize = be16tocpu(tcp->windowSize);
				syn.src.family = ipc::Socket::AF_INET;
				syn.src.d.ipv4.addr = ip->src.value();
				syn.src.d.ipv4.port = be16tocpu(tcp->srcPort);
				// is there already a pending accept?
				if(_pending.count > 0 && (_pending.mid & 0xFFFF) == MSG_DEV_CREATSIBL) {
					forkSocket(_pending.d.accept.nfd,_pending.mid,_pending.d.accept.dev,syn,seqNo);
					_pending.count = 0;
				}
				else {
					if(_rxCircle.push(seqNo,CircularBuf::TYPE_CTRL,&syn,sizeof(SynPacket)) < 0)
						print("Unable to push SYN packet");
				}
			}
		}
		break;

		case STATE_SYN_SENT: {
			if((tcp->ctrlFlags & (TCP::FL_ACK | TCP::FL_SYN)) == (TCP::FL_ACK | TCP::FL_SYN)) {
				size_t txbufSize = be16tocpu(tcp->windowSize);
				txbufSize = std::max<size_t>(1024,std::min<size_t>(64 * 1024,txbufSize));
				_txCircle.init(_txCircle.nextSeq(),txbufSize);
				_rxCircle.init(seqNo + 1,RECV_BUF_SIZE);
				_mss = parseMSS(tcp);

				state(STATE_ESTABLISHED);
				replyPending<int>(0);

				// since this packet is not in our rxCircle, we have to force the ACK
				ackForced = true;
			}
		}
		break;

		case STATE_ESTABLISHED: {
			if(tcp->ctrlFlags & TCP::FL_FIN)
				state(STATE_CLOSE_WAIT);
		}
		break;

		case STATE_SYN_RECEIVED: {
			if(tcp->ctrlFlags & TCP::FL_ACK) {
				// answer the accept call
				assert(_pending.count > 0);
				ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
				ipc::IPCStream is(_pending.d.accept.fd,buffer,sizeof(buffer),_pending.mid);
				is << ipc::FileCreatSibl::Response(0) << ipc::Reply();
				_pending.count = 0;
				state(STATE_ESTABLISHED);
			}
		}
		break;

		case STATE_FIN_WAIT_1: {
			if(tcp->ctrlFlags & TCP::FL_FIN) {
				if(tcp->ctrlFlags & TCP::FL_ACK)
					state(STATE_TIME_WAIT);
				else
					state(STATE_CLOSING);
			}
			else if(tcp->ctrlFlags & TCP::FL_ACK)
				state(STATE_FIN_WAIT_2);
		}
		break;

		case STATE_FIN_WAIT_2: {
			if(tcp->ctrlFlags & TCP::FL_FIN)
				state(STATE_TIME_WAIT);
		}
		break;

		case STATE_CLOSING: {
			if(tcp->ctrlFlags & TCP::FL_ACK)
				state(STATE_TIME_WAIT);
		}
		break;

		case STATE_LAST_ACK: {
			if(tcp->ctrlFlags & TCP::FL_ACK)
				state(STATE_CLOSED);
		}
		break;

		default:
			break;
	}

	// first ACK data and send ACK packet, if required
	if(_state != STATE_CLOSED)
		sendCtrlPkt(TCP::FL_ACK,NULL,ackForced);

	// push data to application if either PSH is set, we don't have much window space left or the
	// state is not ESTABLISHED anymore
	if(tcp->ctrlFlags & TCP::FL_PSH)
		_push = true;
	if(_pending.count > 0 && shouldPush()) {
		replyRead(_pending.mid,_pending.d.read.needsSrc,_pending.d.read.data,_pending.count);
		_pending.count = 0;
	}

	// program timeout, if we went into TIME_WAIT state
	if(oldstate != STATE_TIME_WAIT && _state == STATE_TIME_WAIT)
		Timeouts::program(_timeoutId,std::make_memfun(this,&StreamSocket::timeout),1000);
}

uint16_t StreamSocket::parseMSS(const TCP *tcp) {
	size_t dataOff = (tcp->dataOffset >> 4) * 4;
	size_t optSize = dataOff - sizeof(TCP);
	if(optSize > 0) {
		const uint8_t* options = reinterpret_cast<const uint8_t*>(tcp + 1);
		while(optSize > 0) {
			const OptionHeader *optHead = reinterpret_cast<const OptionHeader*>(options);
			switch(optHead->kind) {
				case OPTION_MSS:
					const MSSOption *mssOpt = reinterpret_cast<const MSSOption*>(optHead);
					return mssOpt->mss;
			}

			options += optHead->length;
			optSize -= optHead->length;
		}
	}
	// use the default size
	return DEF_MSS;
}

ssize_t StreamSocket::sendCtrlPkt(uint8_t flags,MSSOption *opt,bool forceACK) {
	assert(flags != 0);
	CircularBuf::seq_type lastAck = _rxCircle.nextExp();
	CircularBuf::seq_type ack = _rxCircle.getAck();
	size_t optSize = opt ? sizeof(*opt) : 0;

	// automatically ACK the any not-yet-ACKed packets, or if we are forced to send an ACK
	if((flags & ~TCP::FL_ACK) || lastAck != ack || forceACK) {
		if(lastAck != ack)
			flags |= TCP::FL_ACK;
		ssize_t res = TCP::send(remoteIP(),_localPort,remotePort(),flags,opt,optSize,optSize,
			_txCircle.nextSeq(),(flags & TCP::FL_ACK) ? ack : 0,_rxCircle.windowSize());
		if(res < 0)
			return res;
	}

	// do we expect an response?
	if(flags & ~TCP::FL_ACK) {
		// then remember that we've send the control-packed and wait for the ACK
		_txCircle.push(_txCircle.nextSeq(),CircularBuf::TYPE_CTRL,NULL,0);
		_ctrlpkt.flags = flags;
		_ctrlpkt.optSize = optSize;
		if(opt)
			_ctrlpkt.option = *opt;
		_ctrlpkt.timeout = 1000;
		Timeouts::program(_timeoutId,std::make_memfun(this,&StreamSocket::timeout),_ctrlpkt.timeout);
	}
	return 0;
}

void StreamSocket::sendData() {
	if(_txCircle.hasData() > 0) {
		CircularBuf::seq_type seqNo = _txCircle.nextExp();
		CircularBuf::seq_type lastAck = _rxCircle.nextExp();
		CircularBuf::seq_type ackNo = _rxCircle.getAck();
		// TODO allocate that just once
		uint8_t *buf = new uint8_t[_mss];
		size_t left = _remoteWinSize;
		while(left > 0) {
			size_t limit = std::min(left,_mss);
			size_t amount = _txCircle.get(seqNo,buf,limit);
			if(amount == 0)
				break;

			// TODO don't use FL_PSH all the time
			ssize_t res = TCP::send(remoteIP(),_localPort,remotePort(),
				TCP::FL_ACK | TCP::FL_PSH,buf,amount,0,seqNo,ackNo,_rxCircle.windowSize());
			if(res < 0) {
				print("Sending data failed: %s",strerror(res));
				break;
			}

			seqNo += amount;
			left -= amount;
		}
		delete[] buf;

		// no packet sent yet and something to ACK?
		if(left == _remoteWinSize && lastAck != ackNo) {
			seqNo = _txCircle.nextSeq();
			TCP::send(remoteIP(),_localPort,remotePort(),
				TCP::FL_ACK,buf,0,0,seqNo,ackNo,_rxCircle.windowSize());
		}
		else if(left != _remoteWinSize)
			Timeouts::program(_timeoutId,std::make_memfun(this,&StreamSocket::timeout),1000);
	}
}

int StreamSocket::forkSocket(int nfd,msgid_t mid,ipc::ClientDevice<Socket> *dev,SynPacket &syn,
		CircularBuf::seq_type seqNo) {
	StreamSocket *s = new StreamSocket(nfd,ipc::Socket::PROTO_TCP);
	s->_mss = syn.mss;
	s->_remoteAddr = syn.src;
	s->_localPort = _localPort;
	s->state(STATE_SYN_RECEIVED);
	syn.winSize = std::max<size_t>(1024,std::min<size_t>(64 * 1024,syn.winSize));
	s->_txCircle.init(s->_txCircle.nextSeq(),syn.winSize);
	s->_rxCircle.init(seqNo + 1,RECV_BUF_SIZE);
	int res = TCP::addSocket(s,s->_localPort,s->remotePort());
	if(res < 0) {
		delete s;
		return res;
	}

	dev->add(nfd,s);
	s->sendCtrlPkt(TCP::FL_SYN | TCP::FL_ACK);
	s->_pending.count = 1;
	s->_pending.mid = mid;
	s->_pending.d.accept.fd = fd();
	s->_pending.d.accept.nfd = nfd;
	s->_pending.d.accept.dev = dev;
	return 0;
}

void StreamSocket::replyRead(msgid_t mid,bool needsSrc,void *buffer,size_t size) {
	ssize_t res = _rxCircle.capacity() - _rxCircle.windowSize();
	uint8_t *buf = reinterpret_cast<uint8_t*>(buffer);
	if(res > 0) {
		if(buffer == NULL)
			buf = new uint8_t[res];
		res = _rxCircle.pull(buf,size);
		// if there is no additional data, we're done with pushing, if we were anyway
		if(!_rxCircle.hasData())
			_push = false;
	}

	PRINT_TCP(_localPort,remotePort(),"passed %zd bytes to application",res);
	reply(mid,_remoteAddr,needsSrc,buffer ? buffer : NULL,buf,res);
	if(res > 0 && buffer == NULL)
		delete[] buf;
}

const char *StreamSocket::stateName(State st) {
	static const char *names[] = {
		[STATE_CLOSED]			= "STATE_CLOSED",
		[STATE_SYN_SENT]		= "STATE_SYN_SENT",
		[STATE_SYN_RECEIVED]	= "STATE_SYN_RECEIVED",
		[STATE_ESTABLISHED]		= "STATE_ESTABLISHED",
		[STATE_FIN_WAIT_1]		= "STATE_FIN_WAIT_1",
		[STATE_FIN_WAIT_2]		= "STATE_FIN_WAIT_2",
		[STATE_CLOSE_WAIT]		= "STATE_CLOSE_WAIT",
		[STATE_CLOSING]			= "STATE_CLOSING",
		[STATE_LAST_ACK]		= "STATE_LAST_ACK",
		[STATE_TIME_WAIT]		= "STATE_TIME_WAIT",
		[STATE_LISTEN]			= "STATE_LISTEN",
	};
	return names[st];
}
