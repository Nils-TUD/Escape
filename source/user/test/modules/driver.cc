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
#include <esc/io.h>
#include <esc/proc.h>
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/thread.h>
#include <esc/sync.h>
#include <ipc/proto/file.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#include "../modules.h"

#define CLIENT_COUNT	10

typedef struct {
	int fd;
	msgid_t mid;
	tid_t tid;
	ulong buffer[64];
	void *data;
} sTestRequest;

static volatile int closeCount = 0;
static int respId = 1;
static int id;
static tUserSem psem;

static int clientThread(void *arg);
static int getRequests(void *arg);
static int handleRequest(void *arg);
static void printffl(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	usemdown(&psem);
	vprintf(fmt,ap);
	fflush(stdout);
	usemup(&psem);
	va_end(ap);
}

static void sigUsr1(A_UNUSED int sig) {
	signal(SIG_USR1,sigUsr1);
}

int mod_driver(A_UNUSED int argc,A_UNUSED char *argv[]) {
	size_t i;
	if(usemcrt(&psem,1) < 0)
		error("Unable to create lock");

	for(i = 0; i < CLIENT_COUNT; i++) {
		if(startthread(clientThread,NULL) < 0)
			error("Unable to start thread");
	}

	id = createdev("/dev/bla",0666,DEV_TYPE_BLOCK,DEV_OPEN | DEV_READ | DEV_WRITE | DEV_CLOSE);
	if(id < 0)
		error("createdev");

	if(startthread(getRequests,NULL) < 0)
		error("Unable to start thread");

	join(0);
	close(id);
	return EXIT_SUCCESS;
}

static int clientThread(A_UNUSED void *arg) {
	size_t i;
	char buf[12] = {0};
	int fd;
	srand(time(NULL) * gettid());
	do {
		fd = open("/dev/bla",IO_READ | IO_WRITE);
		if(fd < 0)
			sleep(20);
	}
	while(fd < 0);
	printffl("[%d] Reading...\n",gettid());
	if(IGNSIGS(read(fd,buf,sizeof(buf))) < 0)
		error("read");
	printffl("[%d] Got: '%s'\n",gettid(),buf);
	for(i = 0; i < sizeof(buf) - 1; i++)
		buf[i] = (rand() % ('z' - 'a')) + 'a';
	buf[i] = '\0';
	printffl("[%d] Writing '%s'...\n",gettid(),buf);
	if(write(fd,buf,sizeof(buf)) < 0)
		error("write");
	printffl("[%d] Closing...\n",gettid());
	close(fd);
	return EXIT_SUCCESS;
}

static int getRequests(A_UNUSED void *arg) {
	ulong buffer[64];
	int tid;
	if(signal(SIG_USR1,sigUsr1) == SIG_ERR)
		error("Unable to announce signal-handler");

	do {
		msgid_t mid;
		int cfd = getwork(id,&mid,buffer,sizeof(buffer),0);
		if(cfd < 0) {
			if(cfd != -EINTR)
				printe("Unable to get work");
		}
		else {
			sTestRequest *req = (sTestRequest*)malloc(sizeof(sTestRequest));
			req->fd = cfd;
			req->mid = mid;
			memcpy(&req->buffer,buffer,sizeof(buffer));
			req->tid = gettid();
			req->data = NULL;
			if(mid == MSG_FILE_WRITE) {
				ipc::IPCStream is(cfd,buffer,sizeof(buffer));
				ipc::FileWrite::Request r;
				is >> r;

				req->data = malloc(r.count);
				is >> ipc::ReceiveData(req->data,r.count);
			}
			if((tid = startthread(handleRequest,req)) < 0)
				error("Unable to start thread");
		}
	}
	while(closeCount < CLIENT_COUNT);
	printffl("No clients anymore, giving up :(\n");
	return 0;
}

static int handleRequest(void *arg) {
	char resp[12];
	sTestRequest *req = (sTestRequest*)arg;

	ipc::IPCStream is(req->fd,req->buffer,sizeof(req->buffer),req->mid);
	switch(req->mid & 0xFFFF) {
		case MSG_FILE_OPEN: {
			char param[32];
			ipc::FileOpen::Request r(param,sizeof(param));
			is >> r;

			printffl("--[%d,%d] Open: flags=%d\n",gettid(),req->fd,r.flags);

			is << ipc::FileOpen::Response(0) << ipc::Reply();
		}
		break;

		case MSG_FILE_READ: {
			ipc::FileRead::Request r;
			is >> r;

			printffl("--[%d,%d] Read: offset=%u, count=%u\n",gettid(),req->fd,r.offset,r.count);

			is << ipc::FileRead::Response(r.count) << ipc::Reply();
			itoa(resp,sizeof(resp),respId++);
			is << ipc::ReplyData(resp,sizeof(resp));
		}
		break;

		case MSG_FILE_WRITE: {
			ipc::FileWrite::Request r;
			is >> r;

			printffl("--[%d,%d] Write: offset=%u, count=%u, data='%s'\n",gettid(),req->fd,
					r.count,r.offset,req->data);

			is << ipc::FileWrite::Response(r.count) << ipc::Reply();
		}
		break;

		case MSG_FILE_CLOSE: {
			printffl("--[%d,%d] Close\n",gettid(),req->fd);
			closeCount++;
			close(req->fd);
			req->fd = -1;
			if(kill(getpid(),SIG_USR1) < 0)
				error("Unable to send signal to driver-thread");
		}
		break;
	}

	free(req->data);
	free(req);
	return 0;
}
