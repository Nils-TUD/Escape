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
#include <ipc/proto/file.h>
#include <stdlib.h>
#include <stdio.h>

using namespace ipc;

class RandomDevice : public ClientDevice<> {
public:
	explicit RandomDevice(const char *name,mode_t mode)
		: ClientDevice(name,mode,DEV_TYPE_BLOCK,DEV_OPEN | DEV_SHFILE | DEV_READ | DEV_CLOSE) {
		set(MSG_FILE_READ,std::make_memfun(this,&RandomDevice::read));
	}

	void read(IPCStream &is) {
		Client *c = get(is.fd());
		FileRead::Request r;
		is >> r;
		assert(!is.error());

		ssize_t res = 0;
		ushort *data = (r.shmemoff == -1) ? (ushort*)malloc(r.count) : (ushort*)(c->shm() + r.shmemoff);
		if(data) {
			ushort *d = data;
			res = r.count;
			r.count /= sizeof(ushort);
			while(r.count-- > 0)
				*d++ = rand();
		}

		is << FileRead::Response(res) << Reply();
		if(r.shmemoff == -1 && res) {
			is << ReplyData(data,res);
			free(data);
		}
	}
};

int main() {
	srand(time(NULL));
	RandomDevice dev("/dev/random",0444);
	dev.loop();
	return EXIT_SUCCESS;
}
