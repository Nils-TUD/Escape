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
#include <ipc/clientdevice.h>
#include <ipc/ipcstream.h>
#include <stdlib.h>

using namespace ipc;

#define BUF_SIZE		(16 * 1024)

static char zeros[BUF_SIZE];

class ZeroDevice : public ClientDevice<> {
public:
	explicit ZeroDevice(const char *name,mode_t mode)
		: ClientDevice(name,mode,DEV_TYPE_BLOCK,DEV_OPEN | DEV_SHFILE | DEV_READ | DEV_CLOSE) {
		set(MSG_DEV_READ,std::make_memfun(this,&ZeroDevice::read));
	}

	void read(IPCStream &is) {
		Client *c = get(is.fd());
		void *data = NULL;
		size_t offset, count;
		ssize_t shmemoff;
		is >> offset >> count >> shmemoff;
		assert(!is.error());

		if(shmemoff != -1) {
			assert(c->shm() != NULL);
			memset(c->shm() + shmemoff,0,count);
		}
		else {
			data = count <= BUF_SIZE ? zeros : calloc(count,1);
			if(!data) {
				printe("Unable to alloc mem");
				count = 0;
			}
		}

		is << count << Send(MSG_DEV_READ_RESP);
		if(shmemoff == -1 && count) {
			is << SendData(MSG_DEV_READ_RESP,data,count);
			if(count > BUF_SIZE)
				free(data);
		}
	}
};

int main(void) {
	try {
		ZeroDevice dev("/dev/zero",0444);
		dev.loop();
	}
	catch(const IPCException &e) {
		printe("%s",e.what());
	}
	return EXIT_SUCCESS;
}
