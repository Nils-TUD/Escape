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
#include <sys/io.h>
#include <esc/ipc/ipcbuf.h>
#include <utility>

#ifndef IN_KERNEL
#	include <esc/vthrow.h>
#endif

namespace esc {

struct Send;
struct Reply;
struct Receive;
struct SendData;
struct ReplyData;
struct ReceiveData;
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
	friend struct Reply;
	friend struct Receive;
	friend struct SendData;
	friend struct ReplyData;
	friend struct ReceiveData;
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
	 * @param mid the message-id to set (for responses)
	 */
	explicit IPCStream(int f,msgid_t mid = 0)
		: _fd(f), _mid(mid), _buf(new ulong[DEF_SIZE / sizeof(ulong)],DEF_SIZE), _flags(FL_ALLOC) {
	}
	/**
	 * Attaches this IPCStream to the file with given fd and uses the given buffer.
	 *
	 * @param f the file-descriptor
	 * @param buf the buffer (word-aligned!)
	 * @param size the buffer size
	 * @param mid the message-id to set (for responses)
	 */
	explicit IPCStream(int f,ulong *buf,size_t size,msgid_t mid = 0)
		: _fd(f), _mid(mid), _buf(buf,size), _flags() {
	}
	/**
	 * Opens the given device and attaches this IPCStream to it.
	 *
	 * @param dev the device
	 * @param mode the flags for open (O_MSGS by default)
	 * @throws if the operation failed
	 */
	explicit IPCStream(const char *dev,uint mode = O_MSGS)
		: _fd(open(dev,mode)), _mid(), _buf(new ulong[DEF_SIZE / sizeof(ulong)],DEF_SIZE),
		  _flags(FL_OPEN | FL_ALLOC) {
#ifndef IN_KERNEL
		if(_fd < 0)
			VTHROWE("open(" << dev << ")",_fd);
#endif
	}

	/**
	 * No copying; moving only
	 */
	IPCStream(const IPCStream&) = delete;
	IPCStream &operator=(const IPCStream&) = delete;
	IPCStream(IPCStream &&is) : _fd(is._fd), _mid(is._mid), _buf(std::move(is._buf)), _flags(is._flags) {
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
	 * Returns the currently set message id. This has either been got from the last send() call
	 * for the id for the sent message. Or it has been set during construction.
	 *
	 * @return the message-id
	 */
	msgid_t msgid() const {
		return _mid;
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
	msgid_t _mid;
	IPCBuf _buf;
	uint _flags;
};

#ifndef IN_KERNEL
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
#endif

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

	IPCStream &operator()(IPCStream &is) {
		// ensure that we don't send the data that we've received last time
		is.startWriting();
		A_UNUSED ssize_t res = ::send(is.fd(),_mid,is._buf.buffer(),is._buf.pos());
#ifndef IN_KERNEL
		if(EXPECT_FALSE(res < 0))
			VTHROWE("send",res);
#endif
		// remember the id for the next receive
		is._mid = res;
		is.reset();
		return is;
	}

protected:
	msgid_t _mid;
};

/**
 * Can be "put into" a stream to reply the current content of the stream to the file it is attached to.
 * In this case, the message id set in the IPCStream, i.e. normally the one that has been received
 * with the last message, is used.
 */
struct Reply : public Send {
	explicit Reply() : Send(0) {
	}

	IPCStream &operator()(IPCStream &is) {
		_mid = is._mid;
		return Send::operator()(is);
	}
};

/**
 * Can be "put into" a stream to receive a message from the file it is attached to.
 */
struct Receive {
	explicit Receive(bool ignoreSigs = true) : _ignoreSigs(ignoreSigs) {
	}

	IPCStream &operator()(IPCStream &is) {
		ssize_t res;
		do {
			res = ::receive(is.fd(),&is._mid,is._buf.buffer(),is._buf.max());
		}
		while(_ignoreSigs && res == -EINTR);
#ifndef IN_KERNEL
		if(EXPECT_FALSE(res < 0))
			VTHROWE("receive",res);
#endif
		return is;
	}

private:
	bool _ignoreSigs;
};

/**
 * Can be "put into" a stream to send and receive a message from the file it is attached to.
 */
struct SendReceive : public Send, public Receive {
	explicit SendReceive(msgid_t mid = 0,bool ignoreSigs = true) : Send(mid), Receive(ignoreSigs) {
	}

	IPCStream &operator()(IPCStream &is) {
		// TODO use the sendrcv syscall
		Send::operator()(is);
		return Receive::operator()(is);
	}
};

/**
 * Can be "put into" a stream to send plain data (independent of the stream content) to the file it
 * is attached to.
 */
struct SendData {
	explicit SendData(const void *data,size_t size,msgid_t mid) : _mid(mid), _data(data), _size(size) {
	}

	IPCStream &operator()(IPCStream &is) {
		ssize_t res = ::send(is.fd(),_mid,_data,_size);
#ifndef IN_KERNEL
		if(EXPECT_FALSE(res < 0))
			VTHROWE("send",res);
#endif
		is._mid = res;
		return is;
	}

protected:
	msgid_t _mid;
	const void *_data;
	size_t _size;
};

/**
 * Can be "put into" a stream to send plain data (independent of the stream content) to the file it
 * is attached to. Like Reply, it uses the message-id from IPCStream.
 */
struct ReplyData : public SendData {
	explicit ReplyData(const void *data,size_t size) : SendData(data,size,0) {
	}

	IPCStream &operator()(IPCStream &is) {
		_mid = is._mid;
		return SendData::operator()(is);
	}
};

/**
 * Can be "put into" a stream to receive data from the file it is attached to into a given buffer.
 */
struct ReceiveData {
	explicit ReceiveData(void *data,size_t size,bool ignoreSigs = true)
		: _data(data), _size(size), _ignoreSigs(ignoreSigs) {
	}

	IPCStream &operator()(IPCStream &is) {
		ssize_t res;
		do {
			res = ::receive(is.fd(),&is._mid,_data,_size);
		}
		while(_ignoreSigs && res == -EINTR);
#ifndef IN_KERNEL
		if(EXPECT_FALSE(res < 0))
			VTHROWE("receive",res);
#endif
		return is;
	}

private:
	void *_data;
	size_t _size;
	bool _ignoreSigs;
};

class DataBuf {
public:
	explicit DataBuf(char *d,bool free)
		: _data(d), _free(free) {
	}
	explicit DataBuf(size_t size,char *shm,ssize_t shmoff)
		: _data(shmoff == -1 ? new char[size] : shm + shmoff), _free(shmoff == -1) {
	}
	DataBuf(const DataBuf&) = delete;
	DataBuf &operator=(const DataBuf&) = delete;
	~DataBuf() {
		if(_free)
			delete[] _data;
	}

	char *data() {
		return _data;
	}

private:
	char *_data;
	bool _free;
};

static inline IPCStream &operator<<(IPCStream &is,Send op) {
	return op(is);
}
static inline IPCStream &operator<<(IPCStream &is,Reply op) {
	return op(is);
}
static inline IPCStream &operator>>(IPCStream &is,Receive op) {
	return op(is);
}
static inline IPCStream &operator<<(IPCStream &is,SendReceive op) {
	return op(is);
}
static inline IPCStream &operator<<(IPCStream &is,SendData op) {
	return op(is);
}
static inline IPCStream &operator<<(IPCStream &is,ReplyData op) {
	return op(is);
}
static inline IPCStream &operator>>(IPCStream &is,ReceiveData op) {
	return op(is);
}

}
