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

#include <esc/proto/net.h>
#include <esc/stream/istringstream.h>
#include <esc/stream/ostringstream.h>
#include <esc/stream/std.h>
#include <esc/cmdargs.h>
#include <info/arp.h>
#include <sys/common.h>
#include <stdio.h>
#include <stdlib.h>

using namespace esc;

static void usage(const char *name) {
	serr << "Usage: " << name << " (add|rem|show) args...\n";
	serr << "\tadd <ip>  : resolves the given IP and adds it to the ARP table\n";
	serr << "\trem <ip>  : removes the given IP from the ARP table\n";
	serr << "\tshow      : shows all ARP table entries\n";
	exit(EXIT_FAILURE);
}

static Net::IPv4Addr strToIp(const char *str) {
	Net::IPv4Addr ip;
	IStringStream is(str);
	is >> ip;
	return ip;
}

static void writeIp(OStream &os,const Net::IPv4Addr &ip) {
	OStringStream s;
	s << ip;
	os << fmt(s.str(),"-",15);
}

static void arpAdd(Net &net,int argc,char **argv) {
	if(argc < 3)
		usage(argv[0]);

	net.arpAdd(strToIp(argv[2]));
}

static void arpRem(Net &net,int argc,char **argv) {
	if(argc < 3)
		usage(argv[0]);

	net.arpRem(strToIp(argv[2]));
}

int main(int argc,char **argv) {
	Net net("/dev/tcpip");

	if(argc < 2 || strcmp(argv[1],"show") == 0) {
		std::vector<info::arp*> arplist = info::arp::get_list();
		sout << fmt("IP address","-",15) << "MAC address\n";
		for(auto it = arplist.begin(); it != arplist.end(); ++it) {
			writeIp(sout,(*it)->ip());
			sout << (*it)->mac() << "\n";
		}
	}
	else if(strcmp(argv[1],"add") == 0)
		arpAdd(net,argc,argv);
	else if(strcmp(argv[1],"rem") == 0)
		arpRem(net,argc,argv);
	else
		usage(argv[0]);
	return 0;
}
