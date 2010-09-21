/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/lock.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "tdriver.h"

#define MY_LOCK		0x487912ee

typedef struct {
	tFD fd;
	tMsgId mid;
	tTid tid;
	sMsg msg;
	void *data;
} sTestRequest;

static u32 respId = 1;
static sMsg msg;
static tDrvId id;

static int getRequests(void *arg);
static int handleRequest(void *arg);
static void printffl(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	vprintf(fmt,ap);
	lockg(MY_LOCK);
	fflush(stdout);
	unlockg(MY_LOCK);
	va_end(ap);
}

int mod_driver(int argc,char *argv[]) {
	u32 i;
	UNUSED(argc);
	UNUSED(argv);

	for(i = 0; i < 10; i++) {
		if(fork() == 0) {
			char buf[12] = {0};
			srand(time(NULL) * i);
			tFD fd;
			do {
				fd = open("/dev/bla",IO_READ | IO_WRITE);
				if(fd < 0)
					sleep(20);
			}
			while(fd < 0);
			printffl("[%d] Reading...\n",getpid());
			if(RETRY(read(fd,buf,sizeof(buf))) < 0)
				error("read");
			printffl("[%d] Got: '%s'\n",getpid(),buf);
			for(i = 0; i < sizeof(buf) - 1; i++)
				buf[i] = (rand() % ('z' - 'a')) + 'a';
			buf[i] = '\0';
			printffl("[%d] Writing '%s'...\n",getpid(),buf);
			if(write(fd,buf,sizeof(buf)) < 0)
				error("write");
			printffl("[%d] Closing...\n",getpid());
			close(fd);
			return EXIT_SUCCESS;
		}
	}

	id = regDriver("bla",DRV_OPEN | DRV_READ | DRV_WRITE | DRV_CLOSE);
	if(id < 0)
		error("regDriver");
	setDataReadable(id,true);

	if(startThread(getRequests,NULL) < 0)
		error("Unable to start thread");

	join(0);
	unregDriver(id);
	return EXIT_SUCCESS;
}

static int getRequests(void *arg) {
	tMsgId mid;
	while(true) {
		tFD cfd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(cfd < 0)
			printe("[TEST] Unable to get work");
		else {
			sTestRequest *req = (sTestRequest*)malloc(sizeof(sTestRequest));
			req->fd = cfd;
			req->mid = mid;
			memcpy(&req->msg,&msg,sizeof(msg));
			req->tid = gettid();
			req->data = NULL;
			if(mid == MSG_DRV_WRITE) {
				req->data = malloc(msg.args.arg2);
				RETRY(receive(cfd,NULL,req->data,msg.args.arg2));
			}
			if(startThread(handleRequest,req) < 0)
				error("Unable to start thread");
		}
	}
	return 0;
}

static int handleRequest(void *arg) {
	char resp[12];
	sTestRequest *req = (sTestRequest*)arg;
	switch(req->mid) {
		case MSG_DRV_OPEN:
			printffl("--[%d,%d] Open: flags=%d\n",gettid(),req->fd,req->msg.args.arg1);
			req->msg.args.arg1 = 0;
			send(req->fd,MSG_DRV_OPEN_RESP,&req->msg,sizeof(req->msg.args));
			break;
		case MSG_DRV_READ:
			printffl("--[%d,%d] Read: offset=%u, count=%u\n",gettid(),req->fd,
					req->msg.args.arg1,req->msg.args.arg2);
			req->msg.args.arg1 = req->msg.args.arg2;
			req->msg.args.arg2 = true;
			itoa(resp,sizeof(resp),respId++);
			send(req->fd,MSG_DRV_READ_RESP,&req->msg,sizeof(req->msg.args));
			send(req->fd,MSG_DRV_READ_RESP,resp,sizeof(resp));
			break;
		case MSG_DRV_WRITE:
			printffl("--[%d,%d] Write: offset=%u, count=%u, data='%s'\n",gettid(),req->fd,
					req->msg.args.arg1,req->msg.args.arg2,req->data);
			req->msg.args.arg1 = req->msg.args.arg2;
			send(req->fd,MSG_DRV_WRITE_RESP,&req->msg,sizeof(req->msg.args));
			break;
		case MSG_DRV_CLOSE:
			printffl("--[%d,%d] Close\n",gettid(),req->fd);
			break;
		default:
			printffl("--[%d,%d] Unknown command\n",gettid(),req->fd);
			break;
	}
	close(req->fd);
	free(req->data);
	free(req);
	return 0;
}
