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
#include <esc/ipc/device.h>
#include <esc/proto/file.h>
#include <stdio.h>

using namespace ipc;

class NullDevice : public Device {
public:
	explicit NullDevice(const char *name,mode_t mode)
		: Device(name,mode,DEV_TYPE_BLOCK,DEV_READ | DEV_WRITE | DEV_CLOSE) {
		set(MSG_FILE_READ,std::make_memfun(this,&NullDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&NullDevice::write));
	}

	void read(IPCStream &is) {
		is << FileRead::Response(0) << Reply();
	}

	void write(IPCStream &is) {
		/* skip the data-message */
		FileWrite::Request r;
		is >> r >> ReceiveData(NULL,0);

		/* write response and pretend that we've written everything */
		is << FileWrite::Response(r.count) << Reply();
	}
};

int main() {
	NullDevice dev("/dev/null",0666);
	dev.loop();
	return EXIT_SUCCESS;
}
