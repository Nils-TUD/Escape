/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/thread.h>
#include <esc/debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "pingpong.h"

#define DATA_SIZE	4

typedef struct {
	char data[DATA_SIZE];
} sIPCMsg;

static void client(void);
static void server(void);

static int messageCount = 1000;

int mod_pingpong(int argc,char *argv[]) {
	int pid;
	if(argc > 2)
		messageCount = atoi(argv[2]);
	if((pid = fork()) == 0)
		server();
	else {
		client();
		kill(pid,SIG_TERM);
		waitchild(NULL);
	}
	return 0;
}

static void client(void) {
	uint64_t begin,total;
	size_t i;
	sIPCMsg msg;
	int fd;
	do {
		fd = open("/dev/pingpong",IO_MSGS);
		if(fd < 0)
			yield();
	}
	while(fd < 0);

	begin = cpu_rdtsc();
	for(i = 0; i < messageCount; i++) {
		if(send(fd,0,&msg,sizeof(msg)) < 0)
			printe("Message-sending failed");
		if(receive(fd,NULL,&msg,sizeof(msg)) < 0)
			printe("Message-receiving failed");
	}
	total = cpu_rdtsc() - begin;
	printf("Cycles: %Lu, per msg: %Lu\n",total,total / messageCount);
	close(fd);
}

static void server(void) {
	sIPCMsg msg;
	msgid_t mid;
	int fd,dev = createdev("/dev/pingpong",DEV_TYPE_SERVICE,0);
	if(dev < 0)
		error("Unable to create device");
	fd = getwork(&dev,1,NULL,&mid,&msg,sizeof(msg),0);
	while(1) {
		if(send(fd,0,&msg,sizeof(msg)) < 0)
			printe("Message-sending failed");
		if(receive(fd,NULL,&msg,sizeof(msg)) < 0)
			printe("Message-receiving failed");
	}
	close(fd);

	/*while(msg < messageCount) {
		msgid_t mid;
		int fd = getwork(&dev,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			fprintf(stderr,"Unable to get work\n");
		else {
			msg++;
			if(send(fd,0,&msg,sizeof(msg)) < 0)
				printe("Message-sending failed");
			close(fd);
		}
	}*/
	/* don't close the device here, so that the client gets the last response */
}
