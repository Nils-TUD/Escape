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
#include <esc/cmdargs.h>
#include <ipc/proto/net.h>
#include <ipc/proto/socket.h>
#include <stdlib.h>
#include <stdio.h>

using namespace ipc;

static char buffer[512];

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <file> <port>\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	if(isHelpCmd(argc,argv) || argc != 3)
		usage(argv[0]);

	FILE *file = fopen(argv[1],"w");
	if(!file)
		error("Unable to open '%s' for writing",argv[1]);

	Socket sock("/dev/socket",Socket::SOCK_STREAM,Socket::PROTO_TCP);

	ipc::Socket::Addr addr;
	addr.family = ipc::Socket::AF_INET;
	addr.d.ipv4.addr = 0;
	addr.d.ipv4.port = atoi(argv[2]);
	sock.bind(addr);
	sock.listen();

	Socket client = sock.accept();

	uint32_t total;
	client.receive(&total,sizeof(total));

	printf("Reading %u bytes from client...\n",total);
	fflush(stdout);

	ssize_t res;
	while(total > 0 && (res = client.receive(buffer,sizeof(buffer))) > 0) {
		if(fwrite(buffer,1,res,file) != (size_t)res)
			error("fwrite failed");
		total -= res;
	}

	fclose(file);
	return 0;
}
