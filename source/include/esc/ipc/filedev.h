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

#pragma once

#include <sys/common.h>
#include <esc/proto/file.h>
#include <esc/ipc/device.h>
#include <memory>
#include <esc/vthrow.h>

namespace esc {

/**
 * This class is intended to make it easy to provide on-demand-generated files. It supports reading
 * and writing, but needs help by a subclass to actually do the read/write.
 */
class FileDevice : public Device {
public:
	/**
	 * Creates the device
	 *
	 * @param path the path
	 * @param mode the permissions to set
	 */
	explicit FileDevice(const char *path,mode_t mode)
		: Device(path,mode,DEV_TYPE_FILE,DEV_READ | DEV_WRITE | DEV_SIZE) {
		set(MSG_FILE_READ,std::make_memfun(this,&FileDevice::read));
		set(MSG_FILE_WRITE,std::make_memfun(this,&FileDevice::write));
		set(MSG_FILE_SIZE,std::make_memfun(this,&FileDevice::filesize));
	}

	/**
	 * Has to be implemented by the subclass.
	 *
	 * @return the whole content of the file
	 */
	virtual std::string handleRead() = 0;

	/**
	 * Should be implemented by the subclass, if writing should be supported.
	 *
	 * @param offset the offset
	 * @param data the data to write
	 * @param size the number of bytes
	 * @return the number of written bytes (or the error-code)
	 */
	virtual ssize_t handleWrite(size_t,const void *,size_t) {
		return -ENOTSUP;
	}

private:
	void read(IPCStream &is) {
		FileRead::Request r;
		is >> r;
		if(r.offset + (off_t)r.count < r.offset)
			VTHROWE("Invalid offset/count (" << r.offset << "," << r.count << ")",-EINVAL);

		std::string content = handleRead();
		ssize_t res = 0;
		if(r.offset >= content.length())
			res = 0;
		else if(r.offset + r.count > content.length())
			res = content.length() - r.offset;
		else
			res = r.count;

		is << FileRead::Response(res) << Reply();
		if(res)
			is << ReplyData(content.c_str(),res);
	}

	void write(IPCStream &is) {
		FileWrite::Request r;
		is >> r;

		std::unique_ptr<uint8_t[]> data(new uint8_t[r.count]);
		is >> ReceiveData(data.get(),r.count);
		ssize_t res = handleWrite(r.offset,data.get(),r.count);

		is << FileWrite::Response(res) << Reply();
	}

	void filesize(IPCStream &is) {
		std::string content = handleRead();
		is << FileSize::Response(content.length()) << Reply();
	}
};

}
