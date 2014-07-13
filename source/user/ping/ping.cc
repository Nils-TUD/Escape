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
#include <sys/endian.h>
#include <sys/time.h>
#include <sys/thread.h>
#include <esc/proto/net.h>
#include <esc/proto/socket.h>
#include <info/link.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <cmdargs.h>
#include <dns.h>

using namespace ipc;

struct IPv4Header {
	uint8_t versionSize;
	uint8_t typeOfService;
	uint16_t packetSize;
	uint16_t packetId;
	uint16_t fragOffset;
	uint8_t timeToLive;
	uint8_t protocol;
	uint16_t checksum;
	Net::IPv4Addr src;
	Net::IPv4Addr dst;
};

enum {
	MORE_FRAGMENTS	= 0x2000,
	DONT_FRAGMENT	= 0x4000,
};

struct ICMPHeader {
	// header
	uint8_t type;
	uint8_t code;
	uint16_t checksum;

	// data (for echo and echo reply)
	uint16_t identifier;
	uint16_t sequence;
};

enum {
	IP_PROTO_ICMP	= 1
};
enum {
	CMD_ECHO		= 8,
	CMD_ECHO_REPLY	= 0,
};

static uint64_t sendTime;
static uint64_t recvTime;
static volatile bool stop = false;

static void sendEcho(Socket &sock,const Net::IPv4Addr &src,const Net::IPv4Addr &dest,
		size_t nbytes,uint16_t seq,uint ttl) {
	const size_t total = sizeof(IPv4Header) + sizeof(ICMPHeader) + nbytes;
	IPv4Header *ip = static_cast<IPv4Header*>(malloc(total));
	if(!ip)
		error("Not enough memory");

	ICMPHeader *icmp = reinterpret_cast<ICMPHeader*>(ip + 1);
	icmp->code = 0;
	icmp->type = CMD_ECHO;
	icmp->identifier = cputobe16(rand());
	icmp->sequence = cputobe16(seq);
	uint8_t *payload = reinterpret_cast<uint8_t*>(icmp + 1);
	for(size_t i = 0; i < nbytes; ++i)
		payload[i] = i;

	icmp->checksum = 0;
	icmp->checksum = ipc::Net::ipv4Checksum(reinterpret_cast<uint16_t*>(icmp),sizeof(*icmp) + nbytes);

	ip->versionSize = (4 << 4) | 5;
	ip->typeOfService = 0;
	ip->packetSize = cputobe16(total);
	ip->packetId = 0;	// TODO ??
	ip->fragOffset = cputobe16(DONT_FRAGMENT);
	ip->timeToLive = ttl;
	ip->protocol = IP_PROTO_ICMP;
	ip->src = src;
	ip->dst = dest;
	ip->checksum = 0;
	ip->checksum = ipc::Net::ipv4Checksum(reinterpret_cast<uint16_t*>(ip),sizeof(IPv4Header));

	Socket::Addr addr;
	addr.family = Socket::AF_INET;
	addr.d.ipv4.addr = dest.value();
	addr.d.ipv4.port = 0;
	sock.sendto(addr,ip,total);
	sendTime = rdtsc();
	free(ip);
}

static int handleReply(IPv4Header *reply) {
	ICMPHeader *icmp = reinterpret_cast<ICMPHeader*>(reply + 1);
	size_t nbytes = be16tocpu(reply->packetSize) - sizeof(IPv4Header) - sizeof(ICMPHeader);

	if(icmp->type != CMD_ECHO_REPLY)
		return 0;

	recvTime = rdtsc();
	if(ipc::Net::ipv4Checksum(reinterpret_cast<uint16_t*>(reply),sizeof(IPv4Header)) != 0) {
		std::cerr << "IP header has invalid checksum" << std::endl;
		return -1;
	}

	if(ipc::Net::ipv4Checksum(reinterpret_cast<uint16_t*>(icmp),sizeof(*icmp) + nbytes) != 0) {
		std::cerr << "ICMP has invalid checksum" << std::endl;
		return -1;
	}

	return 1;
}

static void sigalarm(int) {
	signal(SIGALRM,sigalarm);
}
static void sigint(int) {
	stop = true;
}

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [options] <address>\n",name);
	fprintf(stderr,"    -c <count>    : perform <count> pings (default: 10)\n");
	fprintf(stderr,"    -s <n>        : use <n> bytes of payload (default: 56)\n");
	fprintf(stderr,"    -t <ttl>      : use <ttl> as time-to-live (default: 64)\n");
	fprintf(stderr,"    -i <interval> : sleep <interval> ms between pings (default: 1000)\n");
	fprintf(stderr,"    -W <timeout>  : wait <timeout> ms for each reply (default: 1000)\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	uint ttl = 64;
	uint nbytes = 56;
	uint times = 10;
	uint interval = 1000;
	uint timeout = 1000;
	const char *address;

	if(signal(SIGALRM,sigalarm) == SIG_ERR)
		error("Unable to set SIGALRM");
	if(signal(SIGINT,sigint) == SIG_ERR)
		error("Unable to set SIGINT");
	srand(time(NULL));

	// parse params
	std::cmdargs args(argc,argv,std::cmdargs::MAX1_FREE);
	try {
		args.parse("c=u s=k t=u i=u W=u",&times,&nbytes,&ttl,&interval,&timeout);
		if(args.is_help())
			usage(argv[0]);
		if(nbytes > 4 * 1024)
			error("The maximum payload size is 4K");
		address = args.get_free().at(0)->c_str();
	}
	catch(const std::cmdargs_error& e) {
		std::cerr << "Invalid arguments: " << e.what() << '\n';
		usage(argv[0]);
	}

	const size_t total = sizeof(IPv4Header) + sizeof(ICMPHeader) + nbytes;
	IPv4Header *reply = static_cast<IPv4Header*>(malloc(total));
	if(!reply)
		error("Not enough memory");

	// get destination
	Net::IPv4Addr dest;
	try {
		dest = std::DNS::getHost(address);
	}
	catch(const std::exception &e) {
		std::cerr << "Unable to resolve '" << address << "': " << e.what() << "\n";
		return EXIT_FAILURE;
	}

	// find the ip address for the corresponding device
	Net::IPv4Addr src;
	{
		char linkname[64];
		Net net("/dev/tcpip");
		net.routeGet(dest,linkname,sizeof(linkname));

		std::vector<info::link*> links = info::link::get_list();
		for(auto it = links.begin(); it != links.end(); ++it) {
			if((*it)->name() == linkname) {
				src = (*it)->ip();
				break;
			}
		}
	}
	if(src == Net::IPv4Addr()) {
		std::cerr << "Unable to find source IP for destination " << dest << "\n";
		return EXIT_FAILURE;
	}

	Socket sock("/dev/socket",Socket::SOCK_RAW_IP,Socket::PROTO_ICMP);
	uint sent = 0;
	uint received = 0;

	std::cout << "PING " << address << " (" << dest << ") " << nbytes;
	std::cout << "(" << total << ") bytes of data." << std::endl;

	uint64_t start = rdtsc();
	for(uint i = 1; !stop && sent < times; ++i) {
		sendEcho(sock,src,dest,nbytes,i,ttl);
		sent++;

		if(alarm(timeout) < 0)
			printe("alarm(%u)",timeout);

		try {
			Socket::Addr addr;
			int res = 0;
			while(res == 0) {
				sock.recvfrom(addr,reply,total);
				res = handleReply(reply);
				if(res == 1) {
					ICMPHeader *icmp = reinterpret_cast<ICMPHeader*>(reply + 1);
					std::cout << be16tocpu(reply->packetSize) << " bytes from " << reply->src << ":";
					std::cout << " icmp_seq=" << be16tocpu(icmp->sequence);
					std::cout << " ttl=" << reply->timeToLive;
					std::cout << " time=" << (tsctotime(recvTime - sendTime) / 1000.0) << " ms" << std::endl;
					received++;
				}
			}

			if(sent < times)
				sleep(interval);
		}
		catch(const std::default_error &e) {
			if(e.error() != -EINTR)
				throw;

			if(!stop) {
				std::cerr << "From " << src << " icmp_seq=" << i;
				std::cerr << " Destination Host Unreachable" << std::endl;
			}
		}
	}
	uint64_t end = rdtsc();

	std::cout << sent << " packets transmitted, " << received << " received, time ";
	std::cout << (tsctotime(end - start) / 1000.0) << "ms\n";
	return 0;
}
