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
#include <string.h>
#include <assert.h>

namespace ipc {

/**
 * Low-level primitive for writing into and reading from a buffer. This is used by the kernel, too,
 * which is the reason why it only provides the basic operations with little convenience. The
 * userspace should use IPCStream.
 */
class IPCBuf {
public:
	/**
	 * Creates a new IPCBuf with given buffer
	 *
	 * @param buf the buffer (word-aligned!)
	 * @param size the size of the buffer
	 */
	explicit IPCBuf(ulong *buf,size_t size) : _buf(buf), _pos(0), _size(size) {
		assert((reinterpret_cast<uintptr_t>(buf) & (sizeof(ulong) - 1)) == 0);
	}

	/**
	 * No copying; moving only
	 */
	IPCBuf(const IPCBuf&) = delete;
	IPCBuf &operator=(const IPCBuf&) = delete;
	IPCBuf(IPCBuf &&is) : _buf(is._buf), _pos(is._pos), _size(is._size) {
		is._buf = NULL;
	}

	/**
	 * @return the buffer
	 */
	ulong *buffer() {
		return _buf;
	}
	const ulong *buffer() const {
		return _buf;
	}

	/**
	 * @return true if an error has occurred during reading or writing
	 */
	bool error() const {
		return _pos > align(_size);
	}
	/**
	 * @return the current position (the number of bytes written or the number of bytes to read)
	 */
	size_t pos() const {
		return _pos;
	}
	/**
	 * @return the maximum capacity of this buffer
	 */
	size_t max() const {
		return _size;
	}

	/**
	 * Resets the position
	 */
	void reset() {
		_pos = 0;
	}

	/**
	 * Writes the given item into the buffer
	 */
	template<typename T>
	IPCBuf &operator<<(const T& value) {
		if(EXPECT_TRUE(checkSpace(sizeof(T)))) {
			// note that we align it before the write to allow the use of the low-level receive()
			// to receive just one item without having to worry about alignment.
			size_t apos = align(_pos);
			*reinterpret_cast<T*>(_buf + apos / sizeof(ulong)) = value;
			_pos = apos + sizeof(T);
		}
		return *this;
	}
	void put(const void *data,size_t size) {
		if(EXPECT_TRUE(checkSpace(size))) {
			size_t apos = align(_pos);
			memcpy(_buf + apos / sizeof(ulong),data,size);
			_pos = apos + size;
		}
	}

	/**
	 * Reads the given item from the buffer
	 */
	template<typename T>
	IPCBuf &operator>>(T &value) {
		if(EXPECT_TRUE(checkSpace(sizeof(T)))) {
			size_t apos = align(_pos);
			value = *reinterpret_cast<T*>(_buf + apos / sizeof(ulong));
			_pos = apos + sizeof(T);
		}
		else
			value = T();
		return *this;
	}
	void fetch(void *data,size_t size) {
		if(EXPECT_TRUE(checkSpace(size))) {
			size_t apos = align(_pos);
			memcpy(data,_buf + apos / sizeof(ulong),size);
			_pos = apos + size;
		}
	}

private:
	static inline size_t align(size_t sz) {
		return (sz + sizeof(ulong) - 1) & ~(sizeof(ulong) - 1);
	}
	bool checkSpace(size_t bytes) {
		if(EXPECT_FALSE(_pos + bytes > _size)) {
			_pos = align(_size) + 1;
			return false;
		}
		return true;
	}

	ulong *_buf;
	size_t _pos;
	size_t _size;
};

/**
 * A class to put a string into an IPCBuf.
 */
struct CString {
	/**
	 * Constructs an empty string
	 */
	explicit CString() : _str(), _len() {
	}
	/**
	 * Uses strlen() to determine the length
	 */
	explicit CString(const char *s) : _str(const_cast<char*>(s)), _len(strlen(s)) {
	}
	/**
	 * Takes the given string and length
	 */
	explicit CString(const char *s,size_t len) : _str(const_cast<char*>(s)), _len(len) {
	}
	explicit CString(char *s,size_t len) : _str(s), _len(len) {
	}

	/**
	 * @return the string
	 */
	char *str() {
		return _str;
	}
	const char *str() const {
		return _str;
	}
	/**
	 * @return the length
	 */
	size_t length() const {
		return _len;
	}

	/**
	 * Write/read operation
	 */
	friend IPCBuf &operator<<(IPCBuf &is,const CString &s) {
		is << s._len;
		is.put(s._str,s._len);
		return is;
	}
	friend IPCBuf &operator>>(IPCBuf &is,CString &s) {
		size_t len;
		is >> len;
		s._len = MIN(len,s._len - 1);
		is.fetch(s._str,s._len);
		s._str[s._len] = '\0';
		return is;
	}

private:
	char *_str;
	size_t _len;
};

/**
 * A class to read a string out of an IPCBuf with <SIZE> bytes at maximum.
 */
template<size_t SIZE>
struct CStringBuf {
	explicit CStringBuf() : _len(SIZE) {
	}

	/**
	 * @return the string
	 */
	char *str() {
		return _str;
	}
	const char *str() const {
		return _str;
	}
	/**
	 * @return the length
	 */
	size_t length() const {
		return _len;
	}

	/**
	 * Read operation
	 */
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
