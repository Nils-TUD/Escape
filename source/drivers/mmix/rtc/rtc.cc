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
#include <esc/time.h>
#include <ipc/proto/rtc.h>
#include <ipc/proto/file.h>
#include <ipc/device.h>
#include <time.h>

class RTCDevice : public ipc::Device {
public:
	explicit RTCDevice(const char *path,mode_t mode)
		: ipc::Device(path,mode,DEV_TYPE_BLOCK,DEV_READ) {
		set(MSG_FILE_READ,std::make_memfun(this,&RTCDevice::read));
	}

	void read(ipc::IPCStream &is) {
		ipc::FileRead::Request r;
		is >> r;

		ssize_t res = r.count;
		if(r.offset + r.count <= r.offset)
			res = -EINVAL;
		else if(r.offset > sizeof(ipc::RTC::Info))
			res = 0;
		else if(r.offset + r.count > sizeof(ipc::RTC::Info))
			res = sizeof(ipc::RTC::Info) - r.offset;

		is << ipc::FileRead::Response(res) << ipc::Send(MSG_FILE_READ_RESP);
		if(res > 0) {
			/* eco32 has no RTC. just report the time since boot */
			ipc::RTC::Info info;
			uint64_t now = tsctotime(rdtsc());
			time_t nowsecs = now / 1000000;
			info.time = *gmtime(&nowsecs);
			info.microsecs = (uint)now % 1000000;
			is << ipc::SendData(MSG_FILE_READ_RESP,(uchar*)&info + r.offset,res);
		}
	}
};

int main(void) {
	RTCDevice dev("/dev/rtc",0444);
	dev.loop();
	return EXIT_SUCCESS;
}
