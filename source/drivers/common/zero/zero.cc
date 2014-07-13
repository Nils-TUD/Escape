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

#include <sys/common.h>
#include <esc/ipc/clientdevice.h>
#include <esc/ipc/ipcstream.h>
#include <esc/proto/file.h>
#include <stdlib.h>

using namespace ipc;

#define BUF_SIZE		(16 * 1024)

static char zeros[BUF_SIZE];

class ZeroDevice : public ClientDevice<> {
public:
	explicit ZeroDevice(const char *name,mode_t mode)
		: ClientDevice(name,mode,DEV_TYPE_BLOCK,DEV_OPEN | DEV_SHFILE | DEV_READ | DEV_CLOSE) {
		set(MSG_FILE_READ,std::make_memfun(this,&ZeroDevice::read));
	}

	void read(IPCStream &is) {
		Client *c = (*this)[is.fd()];
		FileRead::Request r;
		is >> r;

		char *data;
		if(r.shmemoff != -1)
			data = c->shm() + r.shmemoff;
		else if(r.count <= BUF_SIZE)
			data = zeros;
		else
			data = new char[r.count];
		DataBuf buf(data,r.shmemoff == -1 && r.count > BUF_SIZE);

		memset(buf.data(),0,r.count);

		is << FileRead::Response(r.count) << Reply();
		if(r.shmemoff == -1 && r.count)
			is << ReplyData(data,r.count);
	}
};

int main(void) {
	ZeroDevice dev("/dev/zero",0444);
	dev.loop();
	return EXIT_SUCCESS;
}
