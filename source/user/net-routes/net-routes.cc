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
#include <info/route.h>
#include <sys/common.h>
#include <stdio.h>
#include <stdlib.h>

using namespace esc;

static void usage(const char *name) {
	serr << "Usage: " << name << " (add|rem|up|down|show) args...\n";
	serr << "\tadd <link> <dest> <netmask> <gw> : adds the given route\n";
	serr << "\trem <dest>                       : removes  routing table entry <dest>\n";
	serr << "\tup <dest>                        : enables routing table entry <dest>\n";
	serr << "\tdown <dest>                      : disables routing table entry <dest>\n";
	serr << "\tshow                             : shows all routes\n";
	exit(EXIT_FAILURE);
}

static Net::IPv4Addr strToIp(const char *str,bool allowDef = false) {
	Net::IPv4Addr ip;
	if(!allowDef || strcmp(str,"default") != 0) {
		IStringStream is(str);
		is >> ip;
	}
	return ip;
}

static void writeIp(OStream &os,const Net::IPv4Addr &ip) {
	OStringStream s;
	s << ip;
	os << fmt(s.str(),"-",15);
}

static void routeAdd(Net &net,int argc,char **argv) {
	if(argc < 6)
		usage(argv[0]);

	net.routeAdd(argv[2],strToIp(argv[3],true),strToIp(argv[5]),strToIp(argv[4]));
}

static void routeRem(Net &net,int argc,char **argv) {
	if(argc < 3)
		usage(argv[0]);

	net.routeRem(strToIp(argv[2]));
}

static void routeUpDown(Net &net,int argc,char **argv) {
	if(argc != 3)
		usage(argv[0]);

	if(strcmp(argv[1],"up") == 0)
		net.routeConfig(strToIp(argv[2]),Net::UP);
	else
		net.routeConfig(strToIp(argv[2]),Net::DOWN);
}

int main(int argc,char **argv) {
	Net net("/dev/tcpip");

	if(argc < 2 || strcmp(argv[1],"show") == 0) {
		std::vector<info::route*> routes = info::route::get_list();
		sout << fmt("Destination","-",15) << fmt("Gateway","-",15);
		sout << fmt("Mask","-",15) << fmt("Flags","-",6);
		sout << "Link\n";
		for(auto it = routes.begin(); it != routes.end(); ++it) {
			if((*it)->dest() == Net::IPv4Addr())
				sout << fmt("default","-",15);
			else
				writeIp(sout,(*it)->dest());
			if((*it)->gateway() == Net::IPv4Addr())
				sout << fmt("*","-",15);
			else
				writeIp(sout,(*it)->gateway());
			writeIp(sout,(*it)->subnetMask());
			if((*it)->flags() & Net::FL_UP)
				sout << 'U';
			else
				sout << ' ';
			if((*it)->flags() & Net::FL_USE_GW)
				sout << 'G';
			else
				sout << ' ';
			sout << "   ";
			sout << (*it)->link() << "\n";
		}
	}
	else if(strcmp(argv[1],"add") == 0)
		routeAdd(net,argc,argv);
	else if(strcmp(argv[1],"rem") == 0)
		routeRem(net,argc,argv);
	else if(strcmp(argv[1],"up") == 0 || strcmp(argv[1],"down") == 0)
		routeUpDown(net,argc,argv);
	else
		usage(argv[0]);
	return 0;
}
