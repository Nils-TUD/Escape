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
#include <ipc/device.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#include "../modules.h"

class DummyDevice : public ipc::Device {
public:
	explicit DummyDevice(const char *path,mode_t mode) : ipc::Device(path,mode,DEV_TYPE_CHAR,DEV_READ) {
		set(MSG_FILE_READ,std::make_memfun(this,&DummyDevice::read));
	}

	void read(ipc::IPCStream &is) {
		static int resp = 4;
		ipc::FileRead::Request r;
		is >> r;

		is << sizeof(resp) << ipc::Send(MSG_FILE_READ_RESP);
		is << ipc::SendData(MSG_FILE_READ_RESP,&resp,sizeof(resp));
	}
};

static int clientThread(A_UNUSED void *arg) {
	int buf;
	int fd = open("/dev/bla",IO_READ | IO_WRITE);
	if(fd < 0)
		error("Unable to open /dev/bla");
	ssize_t res;
	while((res = read(fd,&buf,sizeof(buf))) > 0)
		;
	printe("Got res=%zd",res);
	close(fd);
	return EXIT_SUCCESS;
}

int mod_driverread(A_UNUSED int argc,A_UNUSED char *argv[]) {
	DummyDevice dev("/dev/bla",0666);

	if(startthread(clientThread,NULL) < 0)
		error("startthread");

	if(fcntl(dev.id(),F_WAKE_READER,0) < 0)
		error("fcntl");
	if(fcntl(dev.id(),F_WAKE_READER,0) < 0)
		error("fcntl");

	char buffer[32];
	for(int i = 0; i < 2; ++i) {
		msgid_t mid;
		int cfd = getwork(dev.id(),&mid,buffer,sizeof(buffer),0);
		if(cfd < 0)
			printe("getwork failed");
		else {
			ipc::IPCStream is(cfd,buffer,sizeof(buffer));
			dev.handleMsg(mid,is);
		}
	}

	fprintf(stderr,"Waiting a second...\n");
	sleep(1000);
	fprintf(stderr,"Closing device\n");
	close(dev.id());
	fprintf(stderr,"Waiting for client to notice it...\n");
	join(0);
	fprintf(stderr,"Shutting down.\n");
	return EXIT_SUCCESS;
}
