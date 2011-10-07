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
#include <esc/thread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "drvparallel.h"

#define MSG_PARA_FIB	10000

typedef struct {
	int n;
	int fd;
} sTask;

static int clientthread(void *arg);
static int fib(int n);
static int fibthread(void *arg);

static size_t clientCount = 10;
static size_t startFib = 20;

int mod_drvparallel(A_UNUSED int argc,A_UNUSED char *argv[]) {
	int i,dev;
	if(argc > 2)
		clientCount = atoi(argv[2]);
	if(argc > 3)
		startFib = atoi(argv[3]);

	dev = createdev("/dev/parallel",DEV_TYPE_SERVICE,0);
	if(dev < 0)
		error("Unable to create device '/dev/parallel'");

	for(i = 0; i < clientCount; i++) {
		if(startThread(clientthread,NULL) < 0)
			error("Unable to start client-thread");
	}

	while(1) {
		sMsg msg;
		msgid_t mid;
		int fd = getWork(&dev,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			fprintf(stderr,"Unable to get work\n");
		else {
			int tid;
			sTask *task = (sTask*)malloc(sizeof(sTask));
			task->n = msg.args.arg1;
			task->fd = fd;
			if((tid = startThread(fibthread,task)) < 0) {
				fprintf(stderr,"Unable to start thread\n");
				close(fd);
				free(task);
			}
			printf("[%d] started\n",tid);
			fflush(stdout);
		}
	}

	close(dev);
	return EXIT_SUCCESS;
}

static int clientthread(A_UNUSED void *arg) {
	int n = startFib;
	int fd = open("/dev/parallel",IO_MSGS);
	while(1) {
		sMsg msg;
		msg.args.arg1 = n;
		send(fd,MSG_PARA_FIB,&msg,sizeof(msg));
		RETRY(receive(fd,NULL,&msg,sizeof(msg)));
		printf("[%d] fib(%d) = %d\n",gettid(),n,msg.args.arg1);
		fflush(stdout);
		n++;
		sleep(200);
	}
	close(fd);
	return 0;
}

static int fib(int n) {
	if(n <= 1)
		return 1;
	return fib(n - 1) + fib(n - 2);
}

static int fibthread(void *arg) {
	sArgsMsg msg;
	sTask *task = (sTask*)arg;
	msg.arg1 = fib(task->n);
	send(task->fd,MSG_DEF_RESPONSE,&msg,sizeof(msg));
	close(task->fd);
	free(task);
	printf("[%d] Done...\n",gettid());
	fflush(stdout);
	return 0;
}
