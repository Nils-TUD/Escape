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
#include <esc/proto/socket.h>
#include <esc/stream/istringstream.h>
#include <esc/stream/std.h>
#include <sys/cmdargs.h>
#include <sys/common.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

using namespace esc;

static char buffer[8192];

static void usage(const char *name) {
	serr << "Usage: " << name << " <ip> <port>\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	if(isHelpCmd(argc,argv) || argc != 3)
		usage(argv[0]);

	esc::Socket::Addr addr;
	addr.family = esc::Socket::AF_INET;
	Socket sock(Socket::SOCK_STREAM,Socket::PROTO_TCP);

	esc::Net::IPv4Addr ip;
	esc::IStringStream is(argv[1]);
	is >> ip;

	addr.d.ipv4.addr = ip.value();
	addr.d.ipv4.port = atoi(argv[2]);
	sock.connect(addr);

	/* send file */
	ssize_t res;
	while((res = read(STDIN_FILENO,buffer,sizeof(buffer))) > 0)
		sock.send(buffer,res);
	return 0;
}
