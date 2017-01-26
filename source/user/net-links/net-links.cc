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
#include <esc/stream/std.h>
#include <esc/cmdargs.h>
#include <info/link.h>
#include <sys/common.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

using namespace esc;

static void usage(const char *name) {
	serr << "Usage: " << name << " (set|up|down|show) args...\n";
	serr << "\tset <link> (ip|subnet) <val>  : configures <link>\n";
	serr << "\tup <link>                     : enables <link>\n";
	serr << "\tdown <link>                   : disables <link>\n";
	serr << "\tshow [<link>]                 : shows all links or <link>\n";
	exit(EXIT_FAILURE);
}

static info::link *getByName(const char *name) {
	std::vector<info::link*> links = info::link::get_list();
	for(auto it = links.begin(); it != links.end(); ++it) {
		if((*it)->name() == name)
			return *it;
	}
	return NULL;
}

static void linkSet(Net &net,int argc,char **argv) {
	if(argc != 5)
		usage(argv[0]);

	info::link *link = getByName(argv[2]);
	if(!link)
		exitmsg("Link '" << argv[2] << "' not found");

	Net::IPv4Addr ip;
	IStringStream is(argv[4]);
	is >> ip;

	if(strcmp(argv[3],"ip") == 0)
		net.linkConfig(argv[2],ip,link->subnetMask(),link->status());
	else if(strcmp(argv[3],"subnet") == 0)
		net.linkConfig(argv[2],link->ip(),ip,link->status());
	else
		usage(argv[0]);
}

static void linkUpDown(Net &net,int argc,char **argv) {
	if(argc != 3)
		usage(argv[0]);

	info::link *link = getByName(argv[2]);
	if(!link)
		exitmsg("Link '" << argv[2] << "' not found");

	if(strcmp(argv[1],"up") == 0)
		net.linkConfig(argv[2],link->ip(),link->subnetMask(),Net::UP);
	else if(strcmp(argv[1],"down") == 0)
		net.linkConfig(argv[2],link->ip(),link->subnetMask(),Net::DOWN);
	else
		usage(argv[0]);
}

static const char *statusNames[] = {
	/* UP */		"UP",
	/* DOWN */		"DOWN",
	/* KILLED */	"KILLED"
};

int main(int argc,char **argv) {
	Net net("/dev/tcpip");

	if(argc < 2 || strcmp(argv[1],"show") == 0) {
		std::vector<info::link*> links = info::link::get_list();
		for(auto it = links.begin(); it != links.end(); ++it) {
			if(argc <= 2 || strcmp(argv[2],(*it)->name().c_str()) == 0) {
				sout << (*it)->name() << ":\n";
				sout << "\tMAC address : " << (*it)->mac() << "\n";
				sout << "\tIPv4 address: " << (*it)->ip() << "\n";
				sout << "\tSubnet mask : " << (*it)->subnetMask() << "\n";
				sout << "\tMTU         : " << (*it)->mtu() << " Bytes\n";
				sout << "\tStatus      : " << statusNames[(*it)->status()] << "\n";
				sout << "\tReceived    : "
						  << (*it)->rxpackets() << " Packets, " << (*it)->rxbytes() << " Bytes\n";
				sout << "\tTransmitted : "
						  << (*it)->txpackets() << " Packets, " << (*it)->txbytes() << " Bytes\n";
			}
		}
	}
	else if(strcmp(argv[1],"set") == 0)
		linkSet(net,argc,argv);
	else if(strcmp(argv[1],"up") == 0 || strcmp(argv[1],"down") == 0)
		linkUpDown(net,argc,argv);
	else
		usage(argv[0]);
	return 0;
}
