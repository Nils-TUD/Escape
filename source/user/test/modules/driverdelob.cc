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

#include <esc/ipc/clientdevice.h>
#include <esc/proto/file.h>
#include <sys/common.h>
#include <sys/driver.h>
#include <sys/io.h>
#include <sys/thread.h>
#include <signal.h>
#include <stdlib.h>

#include "../modules.h"

using namespace esc;

class MyDevice;

static const char *TEST_FILE = "/home/hrniels/testdir/bbc.bmp";

static MyDevice *dev;

class MyDevice : public ClientDevice<> {
public:
	explicit MyDevice(const char *name,mode_t mode)
		: ClientDevice<>(name,mode,DEV_TYPE_BLOCK,DEV_DELEGATE | DEV_OBTAIN | DEV_CLOSE) {
		set(MSG_DEV_DELEGATE,std::make_memfun(this,&MyDevice::delegate));
		set(MSG_DEV_OBTAIN,std::make_memfun(this,&MyDevice::obtain));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&MyDevice::close));
	}

	void delegate(IPCStream &is) {
		DevDelegate::Request r;
		is >> r;

		print("delegate: fd=%d arg=%d",r.nfd,r.arg);

		int res;
		struct stat info;
		if((res = fstat(r.nfd,&info)) < 0) {
			is << DevDelegate::Response(res) << Reply();
			return;
		}
		int perm = fcntl(r.nfd,F_GETACCESS,0);
		if(perm < 0) {
			is << DevDelegate::Response(perm) << Reply();
			return;
		}
		print("inode: %u, dev=%d, perm=%d",info.st_ino,info.st_dev,perm);
		::close(r.nfd);

		is << DevDelegate::Response(0) << Reply();
	}

	void obtain(IPCStream &is) {
		DevObtain::Request r;
		is >> r;

		print("obtain: arg=%d",r.arg);

		int fd = ::open(TEST_FILE,O_RDWR);
		if(fd < 0) {
			is << DevObtain::Response::error(fd) << Reply();
			return;
		}

		print("obtain: -> %d,%u",fd,O_RDONLY);

		is << DevObtain::Response::success(fd,O_RDONLY) << Reply();
	}

	void close(IPCStream &is) {
		ClientDevice<>::close(is);
		stop();
	}
};

#define errornstop(...)		do {	\
		close(fd);					\
		error(#__VA_ARGS__);		\
	} 								\
	while(0)

static int clientThread(void *) {
	int fd = open("/dev/test",O_RDONLY | O_MSGS);
	if(fd < 0)
		error("Unable to open /dev/test");

	if(delegate(fd,fd,O_RDWRMSG,0) < 0)
		errornstop("delegate failed");

	FILE *tmp = tmpfile();
	if(delegate(fd,fileno(tmp),O_RDONLY,0) < 0)
		errornstop("delegate failed");
	fclose(tmp);

	int nfd = open(TEST_FILE,O_RDWR);
	if(nfd < 0)
		printe("Unable to open %s",TEST_FILE);
	else {
		if(delegate(fd,nfd,O_RDONLY,0) < 0)
			errornstop("delegate failed");
		close(nfd);
	}

	int ofd = obtain(fd,0);
	if(ofd < 0)
		errornstop("obtain failed");
	struct stat info;
	if(fstat(ofd,&info) < 0)
		errornstop("stat failed");
	int perm = fcntl(ofd,F_GETACCESS,0);
	if(perm < 0)
		errornstop("fcntl failed");
	print("inode: %u, dev=%d, perm=%d",info.st_ino,info.st_dev,perm);
	close(ofd);

	close(fd);
	return 0;
}

int mod_driverdelob(A_UNUSED int argc,A_UNUSED char *argv[]) {
	dev = new MyDevice("/dev/test",0777);

	if(startthread(clientThread,NULL) < 0)
		error("Unable to cancel thread");

	dev->loop();
	IGNSIGS(join(0));
	return EXIT_SUCCESS;
}
