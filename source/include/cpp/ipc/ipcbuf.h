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

#pragma once

#include <esc/common.h>
#include <string.h>

namespace ipc {

class IPCBuf {
public:
	explicit IPCBuf(char *buf,size_t size) : _buf(buf), _pos(0), _size(size) {
	}

	char *buffer() {
		return _buf;
	}
	const char *buffer() const {
		return _buf;
	}
	bool error() const {
		return _pos > _size;
	}
	size_t pos() const {
		return _pos;
	}
	size_t max() const {
		return _size;
	}

	void reset() {
		_pos = 0;
	}

	template<typename T>
	IPCBuf &operator<<(const T& value) {
		if(EXPECT_TRUE(checkSpace(sizeof(T)))) {
			*reinterpret_cast<T*>(_buf + _pos) = value;
			_pos += sizeof(T);
		}
		return *this;
	}
	void put(const void *data,size_t size) {
		if(EXPECT_TRUE(checkSpace(size))) {
			memcpy(_buf + _pos,data,size);
			_pos += size;
		}
	}

	template<typename T>
	IPCBuf &operator>>(T &value) {
		if(EXPECT_TRUE(checkSpace(sizeof(T)))) {
			value = *reinterpret_cast<T*>(_buf + _pos);
			_pos += sizeof(T);
		}
		else
			value = T();
		return *this;
	}
	void fetch(void *data,size_t size) {
		if(EXPECT_TRUE(checkSpace(size))) {
			memcpy(data,_buf + _pos,size);
			_pos += size;
		}
	}

private:
	bool checkSpace(size_t bytes) {
		if(EXPECT_FALSE(_pos + bytes > _size)) {
			_pos = _size + 1;
			return false;
		}
		return true;
	}

	char *_buf;
	size_t _pos;
	size_t _size;
};

struct CString {
	explicit CString(const char *str) : _str(str), _len(strlen(str)) {
	}
	explicit CString(const char *str,size_t len) : _str(str), _len(len) {
	}

	friend IPCBuf &operator<<(IPCBuf &is,const CString &s) {
		is << s._len;
		is.put(s._str,s._len);
		return is;
	}

private:
	const char *_str;
	size_t _len;
};

template<size_t SIZE>
struct CStringBuf {
	explicit CStringBuf() : _len(SIZE) {
	}

	char *str() {
		return _str;
	}
	const char *str() const {
		return _str;
	}
	size_t length() const {
		return _len;
	}

	friend IPCBuf &operator>>(IPCBuf &is,CStringBuf &s) {
		size_t len;
		is >> len;
		s._len = MIN(len,s._len - 1);
		is.fetch(s._str,s._len);
		s._str[s._len] = '\0';
		return is;
	}

private:
	char _str[SIZE];
	size_t _len;
};

}
