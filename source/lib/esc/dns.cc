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

#include <esc/proto/socket.h>
#include <esc/stream/fstream.h>
#include <esc/stream/istringstream.h>
#include <esc/stream/ostream.h>
#include <esc/dns.h>
#include <sys/common.h>
#include <sys/endian.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <signal.h>

namespace esc {

/* based on http://tools.ietf.org/html/rfc1035 */

#define DNS_RECURSION_DESIRED	0x100
#define DNS_PORT				53
#define BUF_SIZE				512

uint16_t DNS::_nextId = 0;
esc::Net::IPv4Addr DNS::_nameserver;

enum Type {
	TYPE_A		= 1,	/* a host address */
	TYPE_NS		= 2,	/* an authoritative name server */
	TYPE_CNAME	= 5,	/* the canonical name for an alias */
	TYPE_HINFO	= 13,	/* host information */
	TYPE_MX		= 15,	/* mail exchange */
};

enum Class {
	CLASS_IN	= 1		/* the Internet */
};

struct DNSHeader {
	uint16_t id;
	uint16_t flags;
	uint16_t qdCount;
	uint16_t anCount;
	uint16_t nsCount;
	uint16_t arCount;
} A_PACKED;

struct DNSQuestionEnd {
	uint16_t type;
	uint16_t cls;
} A_PACKED;

struct DNSAnswer {
	uint16_t name;
	uint16_t type;
	uint16_t cls;
	uint32_t ttl;
	uint16_t length;
} A_PACKED;

esc::Net::IPv4Addr DNS::getHost(const char *name,uint timeout) {
	if(isIPAddress(name)) {
		esc::Net::IPv4Addr addr;
		IStringStream is(name);
		is >> addr;
		return addr;
	}

	return resolve(name,timeout);
}

bool DNS::isIPAddress(const char *name) {
	int dots = 0;
	int len = 0;
	// ignore whitespace at the beginning
	while(isspace(*name))
		name++;

	while(dots < 4 && len < 4 && *name) {
		if(*name == '.') {
			dots++;
			len = 0;
		}
		else if(isdigit(*name))
			len++;
		else
			break;
		name++;
	}

	// ignore whitespace at the end
	while(isspace(*name))
		name++;
	return dots == 3 && len > 0 && len < 4;
}

esc::Net::IPv4Addr DNS::resolve(const char *name,uint timeout) {
	uint8_t buffer[BUF_SIZE];
	if(_nameserver.value() == 0) {
		FStream in(getResolveFile(),"r");
		in >> _nameserver;
		if(_nameserver.value() == 0)
			VTHROWE("No nameserver",-EHOSTNOTFOUND);
	}

	size_t nameLen = strlen(name);
	size_t total = sizeof(DNSHeader) + nameLen + 2 + sizeof(DNSQuestionEnd);
	if(total > sizeof(buffer))
		VTHROWE("Hostname too long",-EINVAL);

	// generate a unique
	uint16_t txid = (getpid() << 16) | _nextId;

	// build DNS request message
	DNSHeader *h = reinterpret_cast<DNSHeader*>(buffer);
	h->id = cputobe16(txid);
	h->flags = cputobe16(DNS_RECURSION_DESIRED);
	h->qdCount = cputobe16(1);
	h->anCount = 0;
	h->nsCount = 0;
	h->arCount = 0;

	convertHostname(reinterpret_cast<char*>(h + 1),name,nameLen);

	DNSQuestionEnd *qend = reinterpret_cast<DNSQuestionEnd*>(buffer + sizeof(*h) + nameLen + 2);
	qend->type = cputobe16(TYPE_A);
	qend->cls = cputobe16(CLASS_IN);

	// create socket
	esc::Socket sock(esc::Socket::SOCK_DGRAM,esc::Socket::PROTO_UDP);

	// send over socket
	esc::Socket::Addr addr;
	addr.family = esc::Socket::AF_INET;
	addr.d.ipv4.addr = _nameserver.value();
	addr.d.ipv4.port = DNS_PORT;
	sock.sendto(addr,buffer,total);

	sighandler_t oldhandler;
	if((oldhandler = signal(SIGALRM,sigalarm)) == SIG_ERR)
		VTHROW("Unable to set SIGALRM handler");
	int res;
	if((res = ualarm(timeout * 1000)) < 0)
		VTHROWE("ualarm(" << (timeout * 1000) << ")",res);

	try {
		// receive response
		sock.recvfrom(addr,buffer,sizeof(buffer));
	}
	catch(const esc::default_error &e) {
		if(e.error() == -EINTR)
			VTHROWE("Received no response from DNS server " << _nameserver,-ETIMEOUT);

		// ignore errors here
		if(signal(SIGALRM,oldhandler) == SIG_ERR) {}
		throw;
	}

	// ignore errors here
	if(signal(SIGALRM,oldhandler) == SIG_ERR) {}

	if(be16tocpu(h->id) != txid)
		VTHROWE("Received DNS response with wrong transaction id",-EHOSTNOTFOUND);

	int questions = be16tocpu(h->qdCount);
	int answers = be16tocpu(h->anCount);

	// skip questions
	uint8_t *data = reinterpret_cast<uint8_t*>(h + 1);
	for(int i = 0; i < questions; ++i) {
		size_t len = questionLength(data);
		data += len + sizeof(DNSQuestionEnd);
	}

	// parse answers
	for(int i = 0; i < answers; ++i) {
		DNSAnswer *ans = reinterpret_cast<DNSAnswer*>(data);
		if(be16tocpu(ans->type) == TYPE_A && be16tocpu(ans->length) == esc::Net::IPv4Addr::LEN)
			return esc::Net::IPv4Addr(data + sizeof(DNSAnswer));
	}

	VTHROWE("Unable to find IP address in DNS response",-EHOSTNOTFOUND);
}

void DNS::convertHostname(char *dst,const char *src,size_t len) {
	// leave one byte space for the length of the first part
	const char *from = src + len++;
	char *to = dst + len;
	// we start with the \0 at the end
	int partLen = -1;

	for(size_t i = 0; i < len; i++, to--, from--) {
		if(*from == '.') {
			*to = partLen;
			partLen = 0;
		}
		else {
			*to = *from;
			partLen++;
		}
	}
	*to = partLen;
}

size_t DNS::questionLength(const uint8_t *data) {
	size_t total = 0;
	while(*data != 0) {
		uint8_t len = *data;
		// skip this name-part
		total += len + 1;
		data += len + 1;
	}
	// skip zero ending, too
	return total + 1;
}

}
