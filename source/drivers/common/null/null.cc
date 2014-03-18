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
#include <ipc/device.h>
#include <stdio.h>

using namespace ipc;

class NullDevice : public Device {
public:
	explicit NullDevice(const char *name,mode_t mode)
		: Device(name,mode,DEV_TYPE_BLOCK,DEV_READ | DEV_WRITE | DEV_CLOSE) {
		set(MSG_DEV_READ,std::make_memfun(this,&NullDevice::read));
		set(MSG_DEV_WRITE,std::make_memfun(this,&NullDevice::write));
	}

	void read(IPCStream &is) {
		is << static_cast<ssize_t>(0) << Send(MSG_DEV_READ_RESP);
	}

	void write(IPCStream &is) {
		size_t offset,count;
		/* skip the data-message */
		is >> offset >> count >> ReceiveData(NULL,0);

		/* write response and pretend that we've written everything */
		is << count << Send(MSG_DEV_WRITE_RESP);
	}
};

int main() {
	try {
		NullDevice dev("/dev/null",0666);
		dev.loop();
	}
	catch(const IPCException &e) {
		printe("%s",e.what());
	}
	return EXIT_SUCCESS;
}
