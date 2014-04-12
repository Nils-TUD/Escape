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
#include <info/route.h>
#include <ipc/proto/net.h>
#include <cmdargs.h>
#include <stdio.h>
#include <stdlib.h>

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s (add|rem|up|down|show) args...\n",name);
	fprintf(stderr,"\tadd <link> <dest> <netmask> <gw> -- adds the given route\n");
	fprintf(stderr,"\trem <dest>                       -- removes  routing table entry <dest>\n");
	fprintf(stderr,"\tup <dest>                        -- enables routing table entry <dest>\n");
	fprintf(stderr,"\tdown <dest>                      -- disables routing table entry <dest>\n");
	fprintf(stderr,"\tshow                             -- shows all routes\n");
	exit(EXIT_FAILURE);
}

static ipc::Net::IPv4Addr strToIp(const char *str,bool allowDef = false) {
	ipc::Net::IPv4Addr ip;
	if(!allowDef || strcmp(str,"default") != 0) {
		std::istringstream is(str);
		is >> ip;
	}
	return ip;
}

static void writeIp(std::ostream &os,const ipc::Net::IPv4Addr &ip) {
	std::ostringstream s;
	s << ip;
	os << std::setw(15) << s.str();
}

static void routeAdd(ipc::Net &net,int argc,char **argv) {
	if(argc < 6)
		usage(argv[0]);

	net.routeAdd(argv[2],strToIp(argv[3],true),strToIp(argv[5]),strToIp(argv[4]));
}

static void routeRem(ipc::Net &net,int argc,char **argv) {
	if(argc < 3)
		usage(argv[0]);

	net.routeRem(strToIp(argv[2]));
}

static void routeUpDown(ipc::Net &net,int argc,char **argv) {
	if(argc != 3)
		usage(argv[0]);

	if(strcmp(argv[1],"up") == 0)
		net.routeConfig(strToIp(argv[2]),ipc::Net::UP);
	else
		net.routeConfig(strToIp(argv[2]),ipc::Net::DOWN);
}

int main(int argc,char **argv) {
	ipc::Net net("/dev/tcpip");

	if(argc < 2 || strcmp(argv[1],"show") == 0) {
		std::vector<info::route*> routes = info::route::get_list();
		std::cout << std::left;
		std::cout << std::setw(15) << "Destination" << std::setw(15) << "Gateway";
		std::cout << std::setw(15) << "Mask" << std::setw(6) << "Flags";
		std::cout << "Link\n";
		for(auto it = routes.begin(); it != routes.end(); ++it) {
			if((*it)->dest() == ipc::Net::IPv4Addr())
				std::cout << std::setw(15) << "default";
			else
				writeIp(std::cout,(*it)->dest());
			if((*it)->gateway() == ipc::Net::IPv4Addr())
				std::cout << std::setw(15) << "*";
			else
				writeIp(std::cout,(*it)->gateway());
			writeIp(std::cout,(*it)->subnetMask());
			if((*it)->flags() & ipc::Net::FL_UP)
				std::cout << 'U';
			else
				std::cout << ' ';
			if((*it)->flags() & ipc::Net::FL_USE_GW)
				std::cout << 'G';
			else
				std::cout << ' ';
			std::cout << "   ";
			std::cout << (*it)->link() << "\n";
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
