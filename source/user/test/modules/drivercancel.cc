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
#include <esc/driver.h>
#include <esc/thread.h>
#include <ipc/proto/file.h>
#include <ipc/clientdevice.h>
#include <signal.h>
#include <stdlib.h>

#include "../modules.h"

using namespace ipc;

class MyClient : public Client {
public:
	explicit MyClient(int f) : Client(f), mid(0) {
	}

	msgid_t mid;
};

class MyCancelDevice : public ClientDevice<MyClient> {
public:
	explicit MyCancelDevice(const char *name,mode_t mode)
		: ClientDevice<MyClient>(name,mode,DEV_TYPE_BLOCK,DEV_CANCEL | DEV_READ) {
		set(MSG_FILE_READ,std::make_memfun(this,&MyCancelDevice::read));
		set(MSG_DEV_CANCEL,std::make_memfun(this,&MyCancelDevice::cancel));
	}

	void cancel(IPCStream &is) {
		static int count = 0;
		MyClient *c = (*this)[is.fd()];
		msgid_t mid;
		is >> mid;

		if(c->mid != mid)
			is << -EINVAL << Reply();
		else {
			bool answer = count++ > 0;
			if(answer) {
				ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
				IPCStream resp(is.fd(),buffer,sizeof(buffer),c->mid);
				resp << FileRead::Response(4) << Reply() << ReplyData("foo",4);
			}
			is << (answer ? 1 : 0) << Reply();
			c->mid = 0;
		}
	}

	void read(IPCStream &is) {
		MyClient *c = (*this)[is.fd()];
		FileRead::Request r;
		is >> r;

		c->mid = is.msgid();
	}
};

static MyCancelDevice *dev;

static void sigusr1(int) {
	dev->stop();
}
static void sigusr2(int) {
}

static int clientThread(void*) {
	if(signal(SIG_USR2,sigusr2) == SIG_ERR)
		error("Unable to announce SIG_INTRPT handler");

	char buffer[64] = "";
	int fd = open("/dev/cancel",IO_READ);
	if(fd < 0)
		error("Unable to open /dev/cancel");
	ssize_t res = read(fd,buffer,sizeof(buffer));
	if(res < 0)
		printe("Read failed");
	fprintf(stderr,"Got response: %zd: '%s'\n",res,buffer);
	close(fd);
	return 0;
}

static int cancelThread(void*) {
	for(int i = 0; i < 2; ++i) {
		fprintf(stderr,"Starting client thread...\n");
		int tid;
		if((tid = startthread(clientThread,NULL)) < 0)
			error("Unable to start thread");
		fprintf(stderr,"Sleeping a second...\n");
		sleep(1000);
		fprintf(stderr,"Sending signal...\n");
		kill(getpid(),SIG_USR2);
		fprintf(stderr,"Waiting for client thread...\n");
		join(tid);
	}
	kill(getpid(),SIG_USR1);
	return 0;
}

int mod_drivercancel(A_UNUSED int argc,A_UNUSED char *argv[]) {
	dev = new MyCancelDevice("/dev/cancel",0777);

	if(signal(SIG_USR1,sigusr1) == SIG_ERR)
		error("Unable to announce SIG_INTRPT handler");
	if(startthread(cancelThread,NULL) < 0)
		error("Unable to cancel thread");

	dev->loop();
	join(0);
	return EXIT_SUCCESS;
}
