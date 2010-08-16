/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <esc/dir.h>
#include <rawfile.h>
#include <stdexcept>

namespace std {
	rawfile::rawfile()
		: _mode(0), _fd(-1) {
	}
	rawfile::rawfile(tFD fd)
		: _mode(READ | WRITE), _fd(fd) {
	}
	rawfile::rawfile(const string& filename,open_type mode)
		: _mode(0), _fd(-1) {
		open(filename,mode);
	}
	rawfile::~rawfile() {
		close();
	}

	void rawfile::open(const string& filename,open_type mode) {
		close();
		u8 flags = 0;
		if(mode & READ)
			flags |= IO_READ;
		if(mode & WRITE)
			flags |= IO_WRITE | IO_CREATE;
		if(mode & APPEND)
			flags |= IO_APPEND;
		if(mode & TRUNCATE)
			flags |= IO_TRUNCATE;
		char tmp[MAX_PATH_LEN];
		abspath(tmp,sizeof(tmp),filename.c_str());
		_fd = ::open(tmp,flags);
		if(_fd < 0)
			throw io_exception("Unable to open",_fd);
		_mode = mode;
	}
	void rawfile::use(tFD fd) {
		close();
		_fd = fd;
		_mode = READ | WRITE;
	}
	void rawfile::seek(off_type offset,int whence) {
		if(_fd < 0)
			throw io_exception("File not opened",0);
		s32 res;
		if((res = ::seek(_fd,offset,whence)) < 0)
			throw io_exception("Unable to seek",res);
	}
	rawfile::size_type rawfile::read(void *data,size_type size,size_type count) {
		if(_fd < 0 || !(_mode & READ))
			throw io_exception("File not opened for reading",0);
		size_type res = RETRY(::read(_fd,data,size * count));
		if((s32)res < 0)
			throw io_exception("Unable to read",res);
		return res / size;
	}
	rawfile::size_type rawfile::write(const void *data,size_type size,size_type count) {
		if(_fd < 0 || !(_mode & WRITE))
			throw io_exception("File not opened for writing",0);
		size_type res = ::write(_fd,data,size * count);
		if((s32)res < 0)
			throw io_exception("Unable to write",res);
		return res / size;
	}
	void rawfile::close() {
		if(_fd >= 0) {
			::close(_fd);
			_fd = -1;
			_mode = 0;
		}
	}
}
