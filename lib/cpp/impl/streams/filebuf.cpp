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

#include <impl/streams/filebuf.h>
#include <esc/dir.h>
#include <stdio.h>

namespace std {
	filebuf* filebuf::open(const char* s,ios_base::openmode mode) {
		char path[MAX_PATH_LEN];
		close();
		unsigned char omode = getMode(mode);
		abspath(path,sizeof(path),s);
		_fd = ::open(path,omode);
		if(_fd < 0)
			return NULL;
		_mode = mode;
		_totalInPos = 0;
		_inPos = 0;
		_inMax = 0;
		_outPos = 0;
		return this;
	}

	filebuf* filebuf::open(tFD filedesc,ios_base::openmode mode) {
		close();
		_fd = filedesc;
		_mode = mode;
		_totalInPos = 0;
		_inPos = 0;
		_inMax = 0;
		_outPos = 0;
		return this;
	}

	unsigned char filebuf::getMode(ios_base::openmode mode) {
		unsigned char omode = 0;
		if(mode & ios_base::in)
			omode |= IO_READ;
		if(mode & ios_base::out)
			omode |= IO_WRITE | IO_CREATE;
		if(mode & ios_base::trunc)
			omode |= IO_TRUNCATE;
		// TODO ?
		if(mode & (ios_base::app | ios_base::ate))
			omode |= IO_APPEND;
		return omode;
	}

	filebuf* filebuf::close() {
		if(_inBuf) {
			delete[] _inBuf;
			_inBuf = NULL;
		}
		if(_outBuf) {
			flush();
			delete[] _outBuf;
			_outBuf = NULL;
		}
		if(_fd >= 0) {
			::close(_fd);
			_fd = -1;
		}
		return this;
	}

	filebuf::pos_type filebuf::available() const {
		sFileInfo info;
		if(::fstat(_fd,&info) < 0)
			throw bad_state("fstat() failed");
		return info.size - _totalInPos;
	}

	filebuf::char_type filebuf::peek() const {
		if(_fd < 0 || !(_mode & ios_base::in))
			throw bad_state("file not open for reading");
		if(!fillBuffer())
			return EOF;
		return _inBuf[_inPos];
	}
	filebuf::char_type filebuf::get() {
		char_type c = peek();
		_inPos++;
		_totalInPos++;
		return c;
	}
	void filebuf::unget() {
		if(_fd < 0 || !(_mode & ios_base::in))
			throw bad_state("file not open for reading");
		if(_inPos == 0)
			throw bad_state("unget() not possible");
		_inPos--;
		_totalInPos--;
	}

	void filebuf::put(char_type c) {
		if(_fd < 0 || !(_mode & ios_base::out))
			throw bad_state("file not open for writing");
		if(!_outBuf || _outPos >= OUT_BUF_SIZE)
			flush();
		_outBuf[_outPos++] = c;
	}
	void filebuf::flush() {
		if(_fd < 0 || !(_mode & ios_base::out))
			throw bad_state("file not open for writing");
		if(_outBuf && _outPos > 0) {
			long res = ::write(_fd,_outBuf,_outPos);
			if(res < 0)
				throw bad_state("flush() failed");
		}
		else
			_outBuf = new char[OUT_BUF_SIZE];
		_outPos = 0;
	}

	bool filebuf::fillBuffer() const {
		if(_inPos >= _inMax) {
			if(!_inBuf)
				_inBuf = new char[IN_BUF_SIZE];
			_inMax = RETRY(::read(_fd,_inBuf,IN_BUF_SIZE));
			if(_inMax <= 0)
				return false;
			_inPos = 0;
		}
		return true;
	}
}
