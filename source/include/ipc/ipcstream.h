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
#include <esc/io.h>
#include <ipc/ipcbuf.h>
#include <string>

#ifndef IN_KERNEL
#	include <vthrow.h>
#endif

namespace ipc {

struct Send;
struct Receive;
class IPCStream;

static inline IPCStream &operator<<(IPCStream &is,const CString &str);
static inline IPCStream &operator>>(IPCStream &is,CString &str);

template<size_t SIZE>
static inline IPCStream &operator<<(IPCStream &is,const CStringBuf<SIZE> &s);
template<size_t SIZE>
static inline IPCStream &operator>>(IPCStream &is,CStringBuf<SIZE> &s);

/**
 * The higher-level IPC-stream which adds more convenience to IPCBuf and is supposed to be used in
 * userspace.
 */
class IPCStream {
	friend struct Send;
	friend struct Receive;
	friend IPCStream &operator<<(IPCStream &is,const CString &str);
	friend IPCStream &operator>>(IPCStream &is,CString &str);
	template<size_t SIZE>
	friend IPCStream &operator<<(IPCStream &is,const CStringBuf<SIZE> &s);
	template<size_t SIZE>
	friend IPCStream &operator>>(IPCStream &is,CStringBuf<SIZE> &s);

	enum {
		FL_ALLOC	= 1 << 0,
		FL_READING	= 1 << 1,
		FL_OPEN		= 1 << 2,
	};

public:
	static const size_t DEF_SIZE	= 512;

	/**
	 * Attaches this IPCStream to the file with given fd. The buffer is allocated on the heap.
	 *
	 * @param f the file-descriptor
	 */
	explicit IPCStream(int f)
		: _fd(f), _buf(new char[DEF_SIZE],DEF_SIZE), _flags(FL_ALLOC) {
	}
	/**
	 * Attaches this IPCStream to the file with given fd and uses the given buffer.
	 *
	 * @param f the file-descriptor
	 * @param buf the buffer
	 * @param size the buffer size
	 */
	explicit IPCStream(int f,char *buf,size_t size)
		: _fd(f), _buf(buf,size), _flags() {
	}
	/**
	 * Opens the given device and attaches this IPCStream to it.
	 *
	 * @param dev the device
	 * @param mode the flags for open (IO_MSGS by default)
	 * @throws if the operation failed
	 */
	explicit IPCStream(const char *dev,uint mode = IO_MSGS)
		: _fd(open(dev,mode)), _buf(new char[DEF_SIZE],DEF_SIZE), _flags(FL_OPEN | FL_ALLOC) {
#ifndef IN_KERNEL
		if(_fd < 0)
			VTHROWE("open",_fd);
#endif
	}

	/**
	 * Cleans up, i.e. deletes the buffer, closes the device etc., depending on what operations
	 * have been performed in at construction.
	 */
	~IPCStream() {
		if(_flags & FL_ALLOC)
			delete[] _buf.buffer();
		if(_flags & FL_OPEN)
			close(_fd);
	}

	/**
	 * @return the file-descriptor
	 */
	int fd() const {
		return _fd;
	}
	/**
	 * @return true if an error has occurred during reading or writing
	 */
	bool error() const {
		return _buf.error();
	}
	/**
	 * Resets the position
	 */
	void reset() {
		_buf.reset();
	}

	/**
	 * Puts the given item into the stream. Note that this automatically puts the stream into
	 * writing mode, if it is not already in it.
	 */
	template<typename T>
	IPCStream &operator<<(const T& value) {
		startWriting();
		_buf.operator<<(value);
		return *this;
	}
	void put(const void *data,size_t size) {
		startWriting();
		_buf.put(data,size);
	}

	/**
	 * Reads the given item from the stream. Note that this automatically puts the stream into
	 * reading mode, if it is not already in it.
	 */
	template<typename T>
	IPCStream &operator>>(T &value) {
		startReading();
		_buf.operator>>(value);
		return *this;
	}
	void fetch(void *data,size_t size) {
		startReading();
		_buf.fetch(data,size);
	}

private:
	void startWriting() {
		if(EXPECT_FALSE(_flags & FL_READING)) {
			_flags &= ~FL_READING;
			_buf.reset();
		}
	}
	void startReading() {
		if(EXPECT_FALSE(~_flags & FL_READING)) {
			_flags |= FL_READING;
			_buf.reset();
		}
	}

	int _fd;
	IPCBuf _buf;
	uint _flags;
};

static inline IPCStream &operator<<(IPCStream &is,const std::string &str) {
	is << str.length();
	is.put(str.c_str(),str.length());
	return is;
}
static inline IPCStream &operator>>(IPCStream &is,std::string &str) {
	size_t len;
	is >> len;
	str = std::string();
	str.resize(len,' ');
	is.fetch(str.begin(),len);
	return is;
}

static inline IPCStream &operator<<(IPCStream &is,const CString &str) {
	is.startWriting();
	is._buf << str;
	return is;
}
static inline IPCStream &operator>>(IPCStream &is,CString &str) {
	is.startReading();
	is._buf >> str;
	return is;
}

template<size_t SIZE>
static inline IPCStream &operator<<(IPCStream &is,const CStringBuf<SIZE> &s) {
	is.startWriting();
	is._buf << s;
	return is;
}
template<size_t SIZE>
static inline IPCStream &operator>>(IPCStream &is,CStringBuf<SIZE> &s) {
	is.startReading();
	is._buf >> s;
	return is;
}

/**
 * Can be "put into" a stream to send the current content of the stream to the file it is attached to.
 */
struct Send {
	explicit Send(msgid_t mid) : _mid(mid) {
	}

	void operator()(IPCStream &is) {
		// ensure that we don't send the data that we've received last time
		is.startWriting();
		A_UNUSED ssize_t res = ::send(is.fd(), _mid, is._buf.buffer(), is._buf.pos());
#ifndef IN_KERNEL
		if(EXPECT_FALSE(res < 0))
			VTHROWE("send",res);
#endif
		is.reset();
	}

private:
	msgid_t _mid;
};

/**
 * Can be "put into" a stream to receive a message from the file it is attached to.
 */
struct Receive {
	void operator()(IPCStream &is) {
		A_UNUSED ssize_t res = IGNSIGS(::receive(is.fd(), NULL, is._buf.buffer(), is._buf.max()));
#ifndef IN_KERNEL
		if(EXPECT_FALSE(res < 0))
			VTHROWE("receive",res);
#endif
	}
};

/**
 * Can be "put into" a stream to send and receive a message from the file it is attached to.
 */
struct SendReceive : public Send, public Receive {
	explicit SendReceive(msgid_t mid) : Send(mid), Receive() {
	}

	void operator()(IPCStream &is) {
		// TODO use the sendrcv syscall
		Send::operator()(is);
		Receive::operator()(is);
	}
};

/**
 * Can be "put into" a stream to send plain data (independent of the stream content) to the file it
 * is attached to.
 */
struct SendData {
	explicit SendData(msgid_t mid,const void *data,size_t size) : _mid(mid), _data(data), _size(size) {
	}

	void operator()(IPCStream &is) {
		A_UNUSED ssize_t res = ::send(is.fd(), _mid, _data, _size);
#ifndef IN_KERNEL
		if(EXPECT_FALSE(res < 0))
			VTHROWE("send",res);
#endif
	}

private:
	msgid_t _mid;
	const void *_data;
	size_t _size;
};

/**
 * Can be "put into" a stream to receive data from the file it is attached to into a given buffer.
 */
struct ReceiveData {
	explicit ReceiveData(void *data,size_t size) : _data(data), _size(size) {
	}

	void operator()(IPCStream &is) {
		A_UNUSED ssize_t res = IGNSIGS(::receive(is.fd(), NULL, _data, _size));
#ifndef IN_KERNEL
		if(EXPECT_FALSE(res < 0))
			VTHROWE("receive",res);
#endif
	}

private:
	void *_data;
	size_t _size;
};

static inline IPCStream &operator<<(IPCStream &is,Send op) {
	op(is);
	return is;
}
static inline IPCStream &operator>>(IPCStream &is,Receive op) {
	op(is);
	return is;
}
static inline IPCStream &operator<<(IPCStream &is,SendReceive op) {
	op(is);
	return is;
}
static inline IPCStream &operator<<(IPCStream &is,SendData op) {
	op(is);
	return is;
}
static inline IPCStream &operator>>(IPCStream &is,ReceiveData op) {
	op(is);
	return is;
}

}
