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
#include <ipc/exception.h>
#include <string>

namespace ipc {

struct Send;
struct Receive;
class IPCStream;

template<size_t SIZE>
static inline IPCStream &operator<<(IPCStream &is,const CStringBuf<SIZE> &s);
template<size_t SIZE>
static inline IPCStream &operator>>(IPCStream &is,CStringBuf<SIZE> &s);

class IPCStream {
	friend struct Send;
	friend struct Receive;
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

	explicit IPCStream(int f)
		: _fd(f), _buf(new char[DEF_SIZE],DEF_SIZE), _flags(FL_ALLOC) {
	}
	explicit IPCStream(int f,char *buf,size_t size)
		: _fd(f), _buf(buf,size), _flags() {
	}
	explicit IPCStream(const char *dev,uint mode = IO_MSGS)
		: _fd(open(dev,mode)), _buf(new char[DEF_SIZE],DEF_SIZE), _flags(FL_OPEN | FL_ALLOC) {
	}
	~IPCStream() {
		if(_flags & FL_ALLOC)
			delete[] _buf.buffer();
		if(_flags & FL_OPEN)
			close(_fd);
	}

	int fd() const {
		return _fd;
	}
	bool error() const {
		return _buf.error();
	}
	void reset() {
		_buf.reset();
	}

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

template<size_t SIZE>
static inline IPCStream &operator<<(IPCStream &is,const CStringBuf<SIZE> &s) {
	is._buf << s;
	return is;
}
template<size_t SIZE>
static inline IPCStream &operator>>(IPCStream &is,CStringBuf<SIZE> &s) {
	is._buf >> s;
	return is;
}

struct Send {
	explicit Send(msgid_t mid) : _mid(mid) {
	}

	void operator()(IPCStream &is) {
		ssize_t res = ::send(is.fd(), _mid, is._buf.buffer(), is._buf.pos());
		if(EXPECT_FALSE(res < 0))
			throw IPCException("send failed");
		is.reset();
	}

private:
	msgid_t _mid;
};

struct Receive {
	void operator()(IPCStream &is) {
		ssize_t res = ::receive(is.fd(), NULL, is._buf.buffer(), is._buf.max());
		if(EXPECT_FALSE(res < 0))
			throw IPCException("send failed");
	}
};

struct SendReceive : public Send, public Receive {
	explicit SendReceive(msgid_t mid) : Send(mid), Receive() {
	}

	void operator()(IPCStream &is) {
		Send::operator()(is);
		Receive::operator()(is);
	}
};

struct SendData {
	explicit SendData(msgid_t mid,const void *data,size_t size) : _mid(mid), _data(data), _size(size) {
	}

	void operator()(IPCStream &is) {
		ssize_t res = ::send(is.fd(), _mid, _data, _size);
		if(EXPECT_FALSE(res < 0))
			throw IPCException("send failed");
	}

private:
	msgid_t _mid;
	const void *_data;
	size_t _size;
};

struct ReceiveData {
	explicit ReceiveData(void *data,size_t size) : _data(data), _size(size) {
	}

	void operator()(IPCStream &is) {
		ssize_t res = ::receive(is.fd(), NULL, _data, _size);
		if(EXPECT_FALSE(res < 0))
			throw IPCException("receive failed");
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
