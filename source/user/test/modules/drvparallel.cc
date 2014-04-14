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
#include <esc/thread.h>
#include <ipc/device.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../modules.h"

typedef struct {
	int n;
	int fd;
} sTask;

static int clientthread(void *arg);
static int fib(int n);
static int fibthread(void *arg);

class FibDevice : public ipc::Device {
public:
	explicit FibDevice(const char *path,mode_t mode) : ipc::Device(path,mode,DEV_TYPE_SERVICE,0) {
		set(0x1234,std::make_memfun(this,&FibDevice::fib));
	}

	void fib(ipc::IPCStream &is) {
		int n;
		is >> n;

		int tid;
		sTask *task = (sTask*)malloc(sizeof(sTask));
		task->n = n;
		task->fd = is.fd();
		if((tid = startthread(fibthread,task)) < 0) {
			fprintf(stderr,"Unable to start thread\n");
			free(task);
		}
		else {
			printf("[%d] started\n",tid);
			fflush(stdout);
		}
	}
};

static size_t clientCount = 10;
static size_t startFib = 20;

int mod_drvparallel(A_UNUSED int argc,A_UNUSED char *argv[]) {
	if(argc > 2)
		clientCount = atoi(argv[2]);
	if(argc > 3)
		startFib = atoi(argv[3]);

	FibDevice dev("/dev/parallel",0111);
	for(size_t i = 0; i < clientCount; i++) {
		if(startthread(clientthread,NULL) < 0)
			error("Unable to start client-thread");
	}

	dev.loop();
	return EXIT_SUCCESS;
}

static int clientthread(A_UNUSED void *arg) {
	int n = startFib;
	ipc::IPCStream is("/dev/parallel");
	while(1) {
		int res;
		is << n << ipc::SendReceive(0x1234) >> res;
		printf("[%d] fib(%d) = %d\n",gettid(),n,res);
		fflush(stdout);
		n++;
		sleep(200);
	}
	return 0;
}

static int fib(int n) {
	if(n <= 1)
		return 1;
	return fib(n - 1) + fib(n - 2);
}

static int fibthread(void *arg) {
	sTask *task = (sTask*)arg;
	int res = fib(task->n);
	ipc::IPCStream is(task->fd);
	is << res << ipc::Reply();

	free(task);
	printf("[%d] Done...\n",gettid());
	fflush(stdout);
	return 0;
}
