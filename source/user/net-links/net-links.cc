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
#include <info/process.h>
#include <info/link.h>
#include <ipc/proto/net.h>
#include <cmdargs.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define TIMEOUT		2000	/* wait 2 ms for the NIC driver to register the device */

static void linkRem(ipc::Net &net,int argc,char **argv);

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s (add|rem|set|up|down|show) args...\n",name);
	fprintf(stderr,"\tadd <link> <device>             -- adds link <link> offered by <device>\n");
	fprintf(stderr,"\trem <link>                      -- removes link <link>\n");
	fprintf(stderr,"\tset <link> (ip|subnet) <val>    -- configures <link>\n");
	fprintf(stderr,"\tup <link>                       -- enables <link>\n");
	fprintf(stderr,"\tdown <link>                     -- disables <link>\n");
	fprintf(stderr,"\tshow [<link>]                   -- shows all links or <link>\n");
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

static void linkAdd(ipc::Net &net,int argc,char **argv) {
	char path[MAX_PATH_LEN];
	if(argc < 4)
		usage(argv[0]);

	snprintf(path,sizeof(path),"/dev/%s",argv[2]);
	int pid = fork();
	if(pid == 0) {
		const char *args[] = {argv[3],path,NULL};
		exec(argv[3],args);
	}
	else if(pid < 0)
		error("fork");
	else {
		int res;
		uint duration = 0;
		while(duration < TIMEOUT && (res = open(path,IO_READ)) == -ENOENT) {
			sleep(20);
			duration += 20;
			// we want to show the error from open, not sleep
			errno = res;
		}
		if(res < 0)
			error("open");

		try {
			net.linkAdd(argv[2],path);
		}
		catch(const std::exception &e) {
			try {
				linkRem(net,argc,argv);
			}
			catch(...) {
			}
			throw;
		}
	}
}

static void linkRem(ipc::Net &net,int argc,char **argv) {
	if(argc < 3)
		usage(argv[0]);

	net.linkRem(argv[2]);
	std::vector<info::process*> procs = info::process::get_list();
	for(auto it = procs.begin(); it != procs.end(); ++it) {
		// TODO actually, we should validate if it's really a network-driver and not some random
		// process that happens to have that string in its command line ;)
		if((*it)->pid() != getpid() && (*it)->command().find(argv[2])) {
			kill((*it)->pid(),SIG_TERM);
			return;
		}
	}

	printe("Unable to find driver-process for '%s'",argv[2]);
}

static void linkSet(ipc::Net &net,int argc,char **argv) {
	if(argc != 5)
		usage(argv[0]);

	info::link *link = getByName(argv[2]);
	if(!link)
		error("Link '%s' not found",argv[2]);

	ipc::Net::IPv4Addr ip;
	std::istringstream is(argv[4]);
	is >> ip;

	if(strcmp(argv[3],"ip") == 0)
		net.linkConfig(argv[2],ip,link->subnetMask(),link->status());
	else if(strcmp(argv[3],"subnet") == 0)
		net.linkConfig(argv[2],link->ip(),ip,link->status());
	else
		usage(argv[0]);
}

static void linkUpDown(ipc::Net &net,int argc,char **argv) {
	if(argc != 3)
		usage(argv[0]);

	info::link *link = getByName(argv[2]);
	if(!link)
		error("Link '%s' not found",argv[2]);

	if(strcmp(argv[1],"up") == 0)
		net.linkConfig(argv[2],link->ip(),link->subnetMask(),ipc::Net::UP);
	else if(strcmp(argv[1],"down") == 0)
		net.linkConfig(argv[2],link->ip(),link->subnetMask(),ipc::Net::DOWN);
	else
		usage(argv[0]);
}

static const char *statusNames[] = {
	/* UP */		"UP",
	/* DOWN */		"DOWN",
	/* KILLED */	"KILLED"
};

int main(int argc,char **argv) {
	ipc::Net net("/dev/tcpip");

	if(argc < 2 || strcmp(argv[1],"show") == 0) {
		std::vector<info::link*> links = info::link::get_list();
		for(auto it = links.begin(); it != links.end(); ++it) {
			if(argc <= 2 || strcmp(argv[2],(*it)->name().c_str()) == 0) {
				std::cout << (*it)->name() << ":\n";
				std::cout << "\tMAC address : " << (*it)->mac() << "\n";
				std::cout << "\tIPv4 address: " << (*it)->ip() << "\n";
				std::cout << "\tSubnet mask : " << (*it)->subnetMask() << "\n";
				std::cout << "\tMTU         : " << (*it)->mtu() << " Bytes\n";
				std::cout << "\tStatus      : " << statusNames[(*it)->status()] << "\n";
				std::cout << "\tReceived    : "
						  << (*it)->rxpackets() << " Packets, " << (*it)->rxbytes() << " Bytes\n";
				std::cout << "\tTransmitted : "
						  << (*it)->txpackets() << " Packets, " << (*it)->txbytes() << " Bytes\n";
			}
		}
	}
	else if(strcmp(argv[1],"add") == 0)
		linkAdd(net,argc,argv);
	else if(strcmp(argv[1],"rem") == 0)
		linkRem(net,argc,argv);
	else if(strcmp(argv[1],"set") == 0)
		linkSet(net,argc,argv);
	else if(strcmp(argv[1],"up") == 0 || strcmp(argv[1],"down") == 0)
		linkUpDown(net,argc,argv);
	else
		usage(argv[0]);
	return 0;
}
