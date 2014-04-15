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
#include <esc/thread.h>
#include <esc/sync.h>
#include <esc/endian.h>
#include <ipc/proto/socket.h>
#include <ipc/proto/net.h>
#include <iostream>
#include <fstream>
#include <cmdargs.h>
#include <signal.h>
#include <stdlib.h>
#include <dns.h>

using namespace ipc;

/* based on:
 * - http://tools.ietf.org/html/rfc2131
 * - http://tools.ietf.org/html/rfc2132
 */

#define DHCP_MAGIC		0x63825363
#define ETH_10MB		1
#define ETH_10MB_LEN	6
#define DHCP_TIMEOUT	3000 /* ms */

// ports
enum {
	SERVER_PORT	= 67,
	CLIENT_PORT	= 68
};

// BOOTP message types
enum {
	BOOTREQUEST			= 1,
	BOOTREPLY			= 2,
};

// DHCP message types
enum {
	DHCPDISCOVER		= 1,
	DHCPOFFER			= 2,
	DHCPREQUEST			= 3,
	DHCPDECLINE			= 4,
	DHCPACK				= 5,
	DHCPNAK				= 6,
	DHCPRELEASE			= 7,
	DHCPINFORM			= 8,
};

// options
enum {
	OPT_PADDING			= 0,
	OPT_SUBNET_MASK		= 1,
	OPT_ROUTERS			= 3,
	OPT_DNS				= 6,
	OPT_HOSTNAME		= 12,
	OPT_DOMAIN_NAME		= 15,
	OPT_BROADCAST		= 28,
	OPT_REQUESTED_IP	= 50,
	OPT_MSG_TYPE		= 53,
	OPT_SERVER_ADDR		= 54,
	OPT_REQUEST_PARAMS	= 55,
	OPT_END				= 255,
};

// our states
enum State {
	DISCOVER,
	REQUEST,
	SUCCESS,
	FAILED
};

struct DHCPMsg {
	explicit DHCPMsg() {
		memset(this,0,sizeof(*this));
		op = BOOTREQUEST;
		htype = ETH_10MB;
		hlen = ETH_10MB_LEN;
		cookie = cputobe32(DHCP_MAGIC);
	}

	uint8_t op;			/* Message op code / message type. */
	uint8_t htype;		/* Hardware address type */
	uint8_t hlen;		/* Hardware address length */
	uint8_t hops;		/* Client sets to zero, optionally used by relay agents when booting via
						 * a relay agent. */
	uint32_t xid;		/* Transaction ID, a random number chosen by the client, used by the client
						 * and server to associate messages and responses between a client and
						 * a server. */
	uint16_t secs;		/* Filled in by client, seconds elapsed since client began address
						 * acquisition or renewal process. */
	uint16_t flags;		/* Flags */
	uint32_t ciaddr;	/* Client IP address; only filled in if client is in BOUND, RENEW or
						 * REBINDING state and can respond to ARP requests. */
	uint32_t yiaddr;	/* 'your' (client) IP address. */
	uint32_t siaddr;	/* IP address of next server to use in bootstrap; returned in DHCPOFFER,
						 * DHCPACK by server. */
	uint32_t giaddr;	/* Relay agent IP address, used in booting via a relay agent. */
	uint8_t chaddr[16];	/* Client hardware address. */
	uint8_t sname[64];	/* Optional server host name, null terminated string. */
	uint8_t file[128];	/* Boot file name, null terminated string; "generic" name or null in
						 * DHCPDISCOVER, fully qualified directory-path name in DHCPOFFER. */
	uint32_t cookie;
	uint8_t options[308]; /* Optional parameters field not including the cookie. */
} A_PACKED;

struct NetConfig {
	uint32_t xid;
	Net::IPv4Addr serverAddr;
	Net::IPv4Addr ipAddr;
	Net::IPv4Addr broadcast;
	Net::IPv4Addr netmask;
	Net::IPv4Addr router;
	Net::IPv4Addr dnsServer;
};

static void buildDiscover(DHCPMsg *msg,NetConfig *cfg) {
	msg->xid = cfg->xid;

	size_t i = 0;
	msg->options[i++] = OPT_MSG_TYPE;
	msg->options[i++] = 1;
	msg->options[i++] = DHCPDISCOVER;
	msg->options[i++] = OPT_END;
}

static void buildRequest(DHCPMsg *msg,NetConfig *cfg) {
	msg->xid = cfg->xid;

	size_t i = 0;
	msg->options[i++] = OPT_MSG_TYPE;
	msg->options[i++] = 1;
	msg->options[i++] = DHCPREQUEST;

	msg->options[i++] = OPT_REQUESTED_IP;
	msg->options[i++] = Net::IPv4Addr::LEN;
	memcpy(msg->options + i,cfg->ipAddr.bytes(),Net::IPv4Addr::LEN);
	i += Net::IPv4Addr::LEN;

	msg->options[i++] = OPT_SERVER_ADDR;
	msg->options[i++] = Net::IPv4Addr::LEN;
	memcpy(msg->options + i,cfg->serverAddr.bytes(),Net::IPv4Addr::LEN);
	i += Net::IPv4Addr::LEN;

	msg->options[i++] = OPT_REQUEST_PARAMS;
	msg->options[i++] = 4;
	msg->options[i++] = OPT_BROADCAST;
	msg->options[i++] = OPT_ROUTERS;
	msg->options[i++] = OPT_DNS;
	msg->options[i++] = OPT_SUBNET_MASK;

	msg->options[i++] = OPT_END;
}

static uint8_t handleReply(DHCPMsg *msg,NetConfig *cfg) {
	uint8_t msgtype = 255;
	cfg->ipAddr = Net::IPv4Addr(be32tocpu(msg->yiaddr));

	uint8_t *options = msg->options;
	while(*options != OPT_END) {
		if(*options == OPT_PADDING) {
			options++;
			continue;
		}

		uint8_t option = *options;
		options += 2;
		switch(option) {
			case OPT_MSG_TYPE:
				msgtype = *options;
				break;
			case OPT_SUBNET_MASK:
				cfg->netmask = Net::IPv4Addr(options);
				break;
			case OPT_SERVER_ADDR:
				cfg->serverAddr = Net::IPv4Addr(options);
				break;
			case OPT_BROADCAST:
				cfg->broadcast = Net::IPv4Addr(options);
				break;
			case OPT_ROUTERS:
				cfg->router = Net::IPv4Addr(options);
				break;
			case OPT_DNS:
				cfg->dnsServer = Net::IPv4Addr(options);
				break;
		}
		options += options[-1];
	}
	return msgtype;
}

static State findConfig(NetConfig *cfg,const NIC::MAC &mac,uint timeout) {
	// create socket
	Socket sock("/dev/socket",Socket::SOCK_DGRAM,Socket::PROTO_UDP);

	// bind the socket to the client-port, because this is fixed in the DHCP protocol
	ipc::Socket::Addr addr;
	addr.family = Socket::AF_INET;
	addr.d.ipv4.addr = Net::IPv4Addr(0,0,0,0).value();
	addr.d.ipv4.port = CLIENT_PORT;
	sock.bind(addr);

	// build destination address
	addr.family = Socket::AF_INET;
	addr.d.ipv4.addr = Net::IPv4Addr(255,255,255,255).value();
	addr.d.ipv4.port = SERVER_PORT;

	State state = DISCOVER;
	while(state != SUCCESS && state != FAILED) {
		DHCPMsg msg;
		memcpy(msg.chaddr,mac.bytes(),NIC::MAC::LEN);

		switch(state) {
			case DISCOVER:
				std::cout << "Sending DHCPDISCOVER..." << std::endl;
				buildDiscover(&msg,cfg);
				break;

			case REQUEST:
				std::cout << "Requesting IP address " << cfg->ipAddr << "..." << std::endl;
				buildRequest(&msg,cfg);
				break;

			default:
				break;
		}

		sock.sendto(addr,&msg,sizeof(msg));

		// we don't want to wait forever
		alarm(timeout);

		try {
			ipc::Socket::Addr src;
			sock.recvfrom(src,&msg,sizeof(msg));
			uint8_t msgtype = handleReply(&msg,cfg);
			switch(msgtype) {
				case DHCPOFFER:
					std::cout << "Received a DHCPOFFER for IP address " << cfg->ipAddr << std::endl;
					state = REQUEST;
					break;
				case DHCPACK:
					std::cout << "Received a DHCPACK for IP address " << cfg->ipAddr << std::endl;
					state = SUCCESS;
					break;
				case DHCPNAK:
					std::cout << "Request of IP address" << cfg->ipAddr << " failed. Retrying..." << std::endl;
					state = REQUEST;
					break;
				default:
					std::cerr << "Received unknown response " << msgtype << ". Stopping." << std::endl;
					state = FAILED;
					break;
			}
		}
		catch(const std::default_error &e) {
			if(e.error() == -EINTR)
				std::cerr << "Message timed out\n";
			else
				std::cerr << e.what() << "\n";
			state = FAILED;
		}
	}
	return state;
}

static void sigalarm(int) {
}

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-t <timeout>] <link>\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	const char *link = NULL;
	uint timeout = DHCP_TIMEOUT;
	if(argc < 2)
		usage(argv[0]);

	// parse params
	std::cmdargs args(argc,argv,std::cmdargs::MAX1_FREE);
	try {
		args.parse("t=d",&timeout);
		if(args.is_help())
			usage(argv[0]);
		link = args.get_free().at(0)->c_str();
	}
	catch(const std::cmdargs_error& e) {
		std::cerr << "Invalid arguments: " << e.what() << '\n';
		usage(argv[0]);
	}

	srand(time(NULL));
	if(signal(SIG_ALARM,sigalarm) == SIG_ERR)
		error("Unable to set SIG_ALARM handler");

	// connect to network and get mac
	Net net("/dev/tcpip");
	NIC::MAC mac = net.linkMAC(link);

	// TODO actually, we have to remove the complete configuration for the given link first

	// add temporary route to be able to send and receive something
	net.routeAdd(link,Net::IPv4Addr(),Net::IPv4Addr(),Net::IPv4Addr());

	// create a random transaction id
	NetConfig cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.xid = (rand() << 16) | rand();

	std::cout << "Trying to find a configuration for " << link;
	std::cout << " with transaction id 0x" << std::hex << cfg.xid << std::dec << "...\n";
	std::cout << std::endl;

	// execute the DHCP protocol
	State state = FAILED;
	try {
		state = findConfig(&cfg,mac,timeout);
	}
	catch(const std::exception &e) {
		std::cerr << e.what() << "\n";
	}

	// remove temporary route
	net.routeRem(Net::IPv4Addr());

	if(state == SUCCESS) {
		std::cout << "\n";
		std::cout << "Configuration for " << link << ":\n";
		std::cout << "Server     : " << cfg.serverAddr << "\n";
		std::cout << "IP         : " << cfg.ipAddr << "\n";
		std::cout << "Subnet mask: " << cfg.netmask << "\n";
		std::cout << "Broadcast  : " << cfg.broadcast << "\n";
		std::cout << "Router     : " << cfg.router << "\n";
		std::cout << "DNS server : " << cfg.dnsServer << "\n";

		net.linkConfig(link,cfg.ipAddr,cfg.netmask,ipc::Net::UP);
		net.routeAdd(link,cfg.ipAddr.getNetwork(cfg.netmask),Net::IPv4Addr(),cfg.netmask);
		net.routeAdd(link,Net::IPv4Addr(),cfg.router,Net::IPv4Addr());

		std::ofstream os(std::DNS::getResolveFile());
		os << (cfg.dnsServer.value() == 0 ? cfg.router : cfg.dnsServer) << "\n";
	}
	return 0;
}
