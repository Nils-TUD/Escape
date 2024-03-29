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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>

static char buffer[512];

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <port>\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	if(argc != 2)
		usage(argv[0]);

	struct sockaddr_in stSockAddr;
	int sock = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(sock == -1)
		error(EXIT_FAILURE,errno,"socket creation failed");

	memset(&stSockAddr,0,sizeof(stSockAddr));
	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(atoi(argv[1]));
	stSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(sock,(struct sockaddr*)&stSockAddr,sizeof(stSockAddr)) == -1)
		error(EXIT_FAILURE,errno,"bind failed");

	if(listen(sock,1) == -1)
		error(EXIT_FAILURE,errno,"listen failed");

	int cfd = accept(sock,NULL,NULL);
	if(cfd < 0)
		error(EXIT_FAILURE,errno,"accept failed");

	ssize_t res;
	while((res = read(cfd,buffer,sizeof(buffer))) > 0) {
		if(write(STDOUT_FILENO,buffer,res) == -1)
			error(EXIT_FAILURE,errno,"write failed");
	}
	if(res < 0)
		error(EXIT_FAILURE,errno,"read failed");

	if(shutdown(cfd,SHUT_RDWR) == -1)
		error(EXIT_FAILURE,errno,"shutdown failed");

	close(cfd);
	close(sock);
	return EXIT_SUCCESS;
}
