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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace esc;

class RandomDevice : public ClientDevice<> {
public:
	explicit RandomDevice(const char *name,mode_t mode)
		: ClientDevice(name,mode,DEV_TYPE_BLOCK,DEV_OPEN | DEV_SHFILE | DEV_READ | DEV_CLOSE) {
		set(MSG_FILE_READ,std::make_memfun(this,&RandomDevice::read));
	}

	void read(IPCStream &is) {
		Client *c = (*this)[is.fd()];
		FileRead::Request r;
		is >> r;
		assert(!is.error());

		DataBuf buf(r.count,c->shm(),r.shmemoff);
		ushort *d = reinterpret_cast<ushort*>(buf.data());
		ssize_t res = r.count;
		r.count /= sizeof(ushort);
		while(r.count-- > 0)
			*d++ = rand();

		is << FileRead::Response(res) << Reply();
		if(r.shmemoff == -1)
			is << ReplyData(buf.data(),res);
	}
};

int main() {
	srand(time(NULL));
	RandomDevice dev("/dev/random",0444);
	dev.loop();
	return EXIT_SUCCESS;
}
