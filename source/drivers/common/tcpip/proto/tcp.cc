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

#include "ethernet.h"
#include "tcp.h"
#include "ipv4.h"

TCP::socket_map TCP::_socks;

const char *TCP::flagsToStr(uint8_t flags) {
	static const char *names[] = {
		"FIN","SYN","RST","PSH","ACK","URG","???","???"
	};
	static char buf[8 * 4 + 1];
	char *pos = buf;
	for(int i = 0; i < 8; ++i) {
		if(flags & (1 << i)) {
			strcpy(pos,names[i]);
			pos += 3;
			*pos++ = ' ';
		}
	}
	if(pos != buf)
		pos--;
	*pos = '\0';
	return buf;
}

ssize_t TCP::send(const ipc::Net::IPv4Addr &ip,ipc::port_t srcp,ipc::port_t dstp,uint8_t flags,
		const void *data,size_t nbytes,size_t optSize,uint32_t seqNo,uint32_t ackNo,uint16_t winSize) {
	if(nbytes > 0) {
		const size_t total = Ethernet<IPv4<TCP>>().size() + nbytes;
		Ethernet<IPv4<TCP>> *pkt = (Ethernet<IPv4<TCP>>*)malloc(total);
		if(!pkt)
			return -ENOMEM;
		ssize_t res = sendWith(pkt,ip,srcp,dstp,flags,data,nbytes,optSize,seqNo,ackNo,winSize);
		free(pkt);
		return res;
	}
	else {
		Ethernet<IPv4<TCP>> pkt;
		return sendWith(&pkt,ip,srcp,dstp,flags,data,nbytes,optSize,seqNo,ackNo,winSize);
	}
}

ssize_t TCP::sendWith(Ethernet<IPv4<TCP>> *pkt,const ipc::Net::IPv4Addr &ip,ipc::port_t srcp,
		ipc::port_t dstp,uint8_t flags,const void *data,size_t nbytes,size_t optSize,uint32_t seqNo,
		uint32_t ackNo,uint16_t winSize) {
	Route route = Route::find(ip);
	if(!route.valid())
		return -ENETUNREACH;

	const size_t total = Ethernet<IPv4<TCP>>().size() + nbytes;

	PRINT_TCP(srcp,dstp,"sent [%s] seq=%u ack=%u len=%zu win=%u",
		flagsToStr(flags),seqNo,ackNo,nbytes,winSize);

	TCP *tcp = &pkt->payload.payload;
	tcp->srcPort = cputobe16(srcp);
	tcp->dstPort = cputobe16(dstp);
	tcp->seqNumber = cputobe32(seqNo);
	tcp->ackNumber = cputobe32(ackNo);
	// the lower 4 bits are reserved and NS
	tcp->dataOffset = ((sizeof(TCP) + optSize) / 4) << 4;
	tcp->ctrlFlags = flags;
	tcp->windowSize = cputobe16(winSize);
	tcp->urgentPtr = 0;
	memcpy(tcp + 1,data,nbytes);

	tcp->checksum = 0;
	tcp->checksum = ipc::Net::ipv4PayloadChecksum(route.link->ip(),ip,IP_PROTO,
		reinterpret_cast<uint16_t*>(tcp),sizeof(TCP) + nbytes);

	return IPv4<TCP>::sendOver(route,pkt,total,ip,IP_PROTO);
}

void TCP::replyReset(const Ethernet<IPv4<TCP>> *pkt) {
	const IPv4<TCP> *ip = &pkt->payload;
	const TCP *tcp = &pkt->payload.payload;
	size_t tcplen = be16tocpu(ip->packetSize) - IPv4<>().size();
	size_t seglen = tcplen - (tcp->dataOffset >> 4) * 4;

	// sequence number should be the ack number, if present
	uint32_t seqNo = (tcp->ctrlFlags & FL_ACK) ? be32tocpu(tcp->ackNumber) : 0;
	// ack number should be sequence number + segment length
	uint32_t ackNo = be32tocpu(tcp->seqNumber) + seglen;
	send(ip->src,be16tocpu(tcp->dstPort),be16tocpu(tcp->srcPort),FL_RST,NULL,0,0,seqNo,ackNo,0);
}

ssize_t TCP::receive(const std::shared_ptr<Link>&,const Packet &packet) {
	const Ethernet<IPv4<TCP>> *pkt = packet.data<const Ethernet<IPv4<TCP>>*>();
	const IPv4<TCP> *ip = &pkt->payload;
	const TCP *tcp = &ip->payload;
	uint16_t srcp = be16tocpu(tcp->srcPort);
	uint16_t dstp = be16tocpu(tcp->dstPort);

	uint32_t key = getKey(dstp,srcp);
	socket_map::iterator it = _socks.find(key);
	// if there is no socket for the specified remote port, try to find a listening socket on the
	// local port (with remote=0).
	if(it == _socks.end()) {
		key = getKey(dstp,0);
		it = _socks.find(key);
	}

	PRINT_TCP(dstp,srcp,"received [%s] seq=%u ack=%u len=%zu win=%u",
		flagsToStr(tcp->ctrlFlags),be32tocpu(tcp->seqNumber),be32tocpu(tcp->ackNumber),
		be16tocpu(ip->packetSize) - IPv4<>().size() - ((tcp->dataOffset >> 4) * 4),
		be16tocpu(tcp->windowSize));

	if(it != _socks.end()) {
		ipc::Socket::Addr sa;
		sa.family = ipc::Socket::AF_INET;
		sa.d.ipv4.addr = pkt->payload.src.value();
		sa.d.ipv4.port = srcp;
		size_t offset = reinterpret_cast<const uint8_t*>(tcp + 1) - packet.data<uint8_t*>();
		it->second->push(sa,packet,offset);
	}
	// if it is no RST packet, and we have no socket associated with it, send a RST
	else if(~tcp->ctrlFlags & FL_RST)
		replyReset(pkt);
	return 0;
}

void TCP::printSockets(std::ostream &os) {
	for(auto it = _socks.begin(); it != _socks.end(); ++it) {
		Route r = Route::find(it->second->remoteIP());
		os << it->second->fd() << " TCP " << it->second->state() << " ";
		if(r.valid())
			os << r.link->ip();
		else
			os << "?";
		os << ":" << it->second->localPort() << "->";
		os << it->second->remoteIP() << ":" << it->second->remotePort() << "\n";
	}
}
