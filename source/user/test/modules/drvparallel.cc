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

#include <esc/ipc/device.h>
#include <esc/proto/file.h>
#include <sys/common.h>
#include <sys/conf.h>
#include <sys/driver.h>
#include <sys/thread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../modules.h"

static int clientCount = 10;
static int startFib = 20;
static int cpuCount = sysconf(CONF_CPU_COUNT);
static int *tids = new int[cpuCount];

class FibDevice : public esc::Device {
public:
	explicit FibDevice(const char *path,mode_t mode)
			: esc::Device(path,mode,DEV_TYPE_SERVICE,DEV_OPEN), _next(0) {
		set(MSG_FILE_OPEN,std::make_memfun(this,&FibDevice::open));
		set(0x1234,std::make_memfun(this,&FibDevice::fib));
	}

	void open(esc::IPCStream &is) {
		::bindto(is.fd(),tids[_next]);
		_next = (_next + 1) % cpuCount;

		is << esc::FileOpen::Response(0) << esc::Reply();
	}

	void fib(esc::IPCStream &is) {
		int res,n;
		is >> n;

		res = calcFib(n);
		is << res << esc::Reply();
		printf("[%d] Done...\n",gettid());
		fflush(stdout);
	}

private:
	static int calcFib(int n) {
		if(n <= 1)
			return 1;
		return calcFib(n - 1) + calcFib(n - 2);
	}

	size_t _next;
};

static int driverthread(void *arg) {
	FibDevice *dev = (FibDevice*)arg;
	dev->loop();
	return 0;
}

static int clientthread(A_UNUSED void *arg) {
	int n = startFib;
	esc::IPCStream is("/dev/parallel");
	while(1) {
		int res;
		is << n << esc::SendReceive(0x1234) >> res;
		printf("[%d] fib(%d) = %d\n",gettid(),n,res);
		fflush(stdout);
		n++;
		sleep(200);
	}
	return 0;
}

int mod_drvparallel(A_UNUSED int argc,A_UNUSED char *argv[]) {
	if(argc > 2)
		clientCount = atoi(argv[2]);
	if(argc > 3)
		startFib = atoi(argv[3]);

	FibDevice dev("/dev/parallel",0111);
	for(int i = 0; i < cpuCount; ++i) {
		tids[i] = startthread(driverthread,&dev);
		if(tids[i] < 0)
			error("Unable to start client-thread");
		if(i == 0)
			bindto(dev.id(),tids[i]);
	}
	for(int i = 0; i < clientCount; i++) {
		if(startthread(clientthread,NULL) < 0)
			error("Unable to start client-thread");
	}

	join(0);
	return EXIT_SUCCESS;
}
