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

#include <esc/stream/ostream.h>
#include <string>

namespace esc {

/**
 * Output-stream that writes to a buffer. The buffer will always be null-terminated after writing
 * to it.
 */
class OBufStream : public OStream {
public:
	/**
	 * Constructor
	 *
	 * @param buf the buffer to write to
	 * @param size the size of the buffer
	 */
	explicit OBufStream(char *buf,size_t size) : OStream(), _buf(buf), _pos(), _size(size) {
	}

	OBufStream(OBufStream &&os)
		: OStream(std::move(os)), _buf(os._buf), _pos(os._pos), _size(os._size) {
	}
	OBufStream &operator=(OBufStream &&os) {
		if(&os != this) {
			OStream::operator=(std::move(os));
			_buf = os._buf;
			_pos = os._pos;
			_size = os._size;
		}
		return *this;
	}

	/**
	 * @return the number of written characters
	 */
	size_t length() const {
		return _pos;
	}

	virtual void write(char c) override {
		if(_pos + 1 < _size) {
			_buf[_pos++] = c;
			_buf[_pos] = '\0';
		}
	}
	virtual size_t write(const void *src,size_t count) override {
		if(_pos + count < _pos || _pos + count > _size)
			count = _size - _pos;
		memcpy(_buf + _pos,src,count);
		_pos += count;
		return count;
	}

private:
	char *_buf;
	size_t _pos;
	size_t _size;
};

}
