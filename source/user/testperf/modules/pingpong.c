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
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/thread.h>
#include <esc/debug.h>
#include <esc/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "../modules.h"

#define DATA_SIZE	4

typedef struct {
	char data[DATA_SIZE];
} sIPCMsg;

static void client(void);
static void server(void);
static void server_fast(void);
static void send_recv_alone(void);

static size_t messageCount = 100000;

int mod_sendrecv(int argc,char *argv[]) {
	int pid;
	if(argc > 2)
		messageCount = atoi(argv[2]);
	send_recv_alone();
	if((pid = fork()) == 0)
		server_fast();
	else {
		client();
		if(kill(pid,SIG_TERM) < 0)
			perror("kill");
		waitchild(NULL);
	}
	return 0;
}

int mod_pingpong(int argc,char *argv[]) {
	int pid;
	if(argc > 2)
		messageCount = atoi(argv[2]);
	if((pid = fork()) == 0)
		server();
	else {
		client();
		if(kill(pid,SIG_TERM) < 0)
			perror("kill");
		waitchild(NULL);
	}
	return 0;
}

static void client(void) {
	uint64_t begin,end;
	size_t i;
	sIPCMsg msg;
	int fd;
	do {
		fd = open("/dev/pingpong",IO_MSGS);
		if(fd < 0)
			yield();
	}
	while(fd < 0);

	msgid_t mid = 0;
	begin = rdtsc();
	for(i = 0; i < messageCount; i++) {
		if(sendrecv(fd,&mid,&msg,sizeof(msg)) < 0)
			printe("sendrecv failed");
	}
	end = rdtsc();
	printf("%Lu cycles, per msg: %Lu\n",end - begin,(end - begin) / messageCount);
	close(fd);
}

static void server(void) {
	sIPCMsg msg;
	int dev = createdev("/dev/pingpong",0111,DEV_TYPE_SERVICE,DEV_CLOSE);
	if(dev < 0) {
		printe("Unable to create device");
		return;
	}
	while(1) {
		msgid_t mid;
		int fd = getwork(dev,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			fprintf(stderr,"Unable to get work\n");
		else {
			if(send(fd,0,&msg,sizeof(msg)) < 0)
				printe("Message-sending failed");
		}
	}
}

static void server_fast(void) {
	sIPCMsg msg;
	msgid_t mid;
	int fd,dev = createdev("/dev/pingpong",0111,DEV_TYPE_SERVICE,DEV_CLOSE);
	if(dev < 0) {
		printe("Unable to create device");
		return;
	}
	fd = getwork(dev,&mid,&msg,sizeof(msg),0);
	while(1) {
		if(sendrecv(fd,&mid,&msg,sizeof(msg)) < 0)
			printe("sendrecv failed");
	}
}

static void send_recv_alone(void) {
	sIPCMsg msg;
	uint64_t begin,end;
	const size_t testcount = 1000;
	int dev = createdev("/dev/pingpong",0111,DEV_TYPE_SERVICE,DEV_CLOSE);
	if(dev < 0) {
		printe("Unable to create device");
		return;
	}

	int fd = open("/dev/pingpong",IO_MSGS);
	if(fd < 0) {
		printe("Unable to open device");
		return;
	}

	begin = rdtsc();
	for(size_t i = 0; i < testcount; i++) {
		if(send(fd,0,&msg,sizeof(msg)) < 0)
			printe("Message-sending failed");
	}
	end = rdtsc();
	printf("send   : %Lu cycles, per call: %Lu\n",end - begin,(end - begin) / testcount);

	int cfd;
	while((cfd = getwork(dev,NULL,&msg,sizeof(msg),GW_NOBLOCK)) >= 0)
		send(cfd,MSG_DEF_RESPONSE,&msg,sizeof(msg));

	begin = rdtsc();
	for(size_t i = 0; i < testcount; i++) {
		if(receive(fd,0,&msg,sizeof(msg)) < 0)
			printe("Message-sending failed");
	}
	end = rdtsc();
	printf("receive: %Lu cycles, per call: %Lu\n",end - begin,(end - begin) / testcount);

	close(fd);
	close(dev);
}
