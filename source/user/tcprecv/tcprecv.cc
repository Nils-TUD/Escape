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
#include <esc/stream/std.h>
#include <sys/common.h>
#include <sys/thread.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

using namespace esc;

static char buffer[8192];

static void usage(const char *name) {
	serr << "Usage: " << name << " <port>\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	if(getopt_ishelp(argc,argv) || argc != 2)
		usage(argv[0]);

	Socket sock(Socket::SOCK_STREAM,Socket::PROTO_TCP);

	esc::Socket::Addr addr;
	addr.family = esc::Socket::AF_INET;
	addr.d.ipv4.addr = 0;
	addr.d.ipv4.port = atoi(argv[1]);
	sock.bind(addr);
	sock.listen();

	Socket client = sock.accept();

	ssize_t res;
	while((res = client.receive(buffer,sizeof(buffer))) > 0) {
		if(write(STDOUT_FILENO,buffer,res) < 0)
			exitmsg("write failed");
	}
	return 0;
}
