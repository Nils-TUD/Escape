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

#include <esc/common.h>
#include <ipc/proto/socket.h>
#include <algorithm>
#include <ostream>
#include <limits>
#include <list>
#include <errno.h>
#include <string.h>
#include <assert.h>

/**
 * +----------------+                  \
 * |                |                  |
 * |   unreached    |                  |- free
 * |                |                  |
 * +-------vv-------+ <-- seqStart     /  <-- _packets
 * |                |
 * |     acked      |
 * |                |
 * +-------vv-------+ <-- seqAcked
 * |                |
 * |    not yet     |
 * |   contiguous   |
 * |                |
 * +-------vv-------+                  \ <-- _packets + _max
 * |                |                  |
 * |   unreached    |                  |- free
 * |                |                  |
 * +----------------+                  /
 */

class CircularBuf;
static std::ostream &operator<<(std::ostream &os,const CircularBuf &cb);

/**
 * This class implements a circular buffer required for TCP. It is used for both the sending and
 * the receiving side.
 * For sending, we push() sent data into the buffer in order to know what to resend if a timeout
 * triggers. To do so, we use get() to get the already sent data again. When we receive the ACK,
 * we forget() the data.
 * For receiving, we push() received data into the buffer. When sending the next packet, we use
 * getAck() to ACK the data. Later we use pull() to pull the data out of the circular buffer and
 * pass it to the application.
 * The data is kept in a std::list of SeqPacket, ordered by the sequence number. Additionally, we
 * keep the start of the window (_seqStart) and the ACK position (_seqAcked).
 */
class CircularBuf {
	friend std::ostream &operator<<(std::ostream &os,const CircularBuf &cb);

public:
	typedef uint32_t seq_type;

	enum {
		TYPE_CTRL,
		TYPE_DATA
	};

	/**
	 * A packet that was pushed. Holds the data with the associated sequence number
	 */
	struct SeqPacket {
		explicit SeqPacket(seq_type _start,uint8_t _type,const uint8_t *_data,size_t sz)
			: start(_start), type(_type), data(sz ? new uint8_t[sz] : NULL), _size(sz) {
			memcpy(data,_data,_size);
		}
		SeqPacket(const SeqPacket&) = delete;
		SeqPacket &operator=(const SeqPacket&) = delete;
		SeqPacket(SeqPacket &&p) : start(p.start), type(p.type), data(p.data), _size(p._size) {
			p.data = NULL;
		}
		~SeqPacket() {
			delete[] data;
		}

		size_t size() const {
			return type == TYPE_CTRL ? 1 : _size;
		}

		seq_type start;
		uint8_t type;
		uint8_t *data;
		size_t _size;
	};

	/**
	 * Creates an uninitialized circular buffer, i.e. with sequence number 0.
	 */
	explicit CircularBuf() : _max(), _current(), _packets(), _seqStart(), _seqAcked() {
	}

	/**
	 * Inits the circular buffer.
	 *
	 * @param start the initial sequence number to use
	 * @param size the maximum number of bytes to hold
	 */
	void init(seq_type start,size_t size) {
		_seqStart = _seqAcked = start;
		_max = size;
	}

	/**
	 * @return the packet list
	 */
	const std::list<SeqPacket> &packets() const {
		return _packets;
	}
	/**
	 * @return the number of bytes to pull()
	 */
	size_t available() const {
		return _current;
	}
	/**
	 * @return the total capacity
	 */
	size_t capacity() const {
		return _max;
	}
	/**
	 * @return the current window size, i.e. the space left
	 */
	size_t windowSize() const {
		return _max - _current;
	}
	/**
	 * @return the expected next sequence number, i.e. the ACK position
	 */
	seq_type nextExp() const {
		return _seqAcked;
	}
	/**
	 * @return the next sequence number that is used (meaningless for the receive buffer)
	 */
	seq_type nextSeq() const {
		return _seqAcked + _current;
	}
	/**
	 * @param seqNo the sequence number
	 * @return if the given sequence number is in the window
	 */
	bool isInWindow(seq_type seqNo) const {
		seq_type relSeq = seqNo - _seqStart;
		return relSeq <= _current;
	}

	/**
	 * Pushes the given data at given position into the buffer. This might fail if the position is
	 * completely outside the window. If the start or the end position is inside the window, some
	 * data will be kept, some will be ignored.
	 *
	 * @param seqNo the sequence number of the first byte of the data
	 * @param type the type (TYPE_{CTRL,DATA})
	 * @param data the data
	 * @param size the number of bytes
	 * @return the number of inserted bytes
	 */
	ssize_t push(seq_type seqNo,uint8_t type,const void *data,size_t size);

	/**
	 * ACKs all data that can be ACKed and returns the new ACK position.
	 *
	 * @return the new ACK position
	 */
	seq_type getAck();

	/**
	 * Forgets all data up to <seqNo>. That is, the data is ACKed and pulled to /dev/null.
	 *
	 * @param seqNo the sequence number
	 * @return 0 on success (if the sequence number is in the window)
	 */
	int forget(seq_type seqNo);

	/**
	 * Gets already pushed data into <buf>. That is, it copies as much data as possible beginning
	 * at <seqNo> into <buf>.
	 *
	 * @param seqNo the sequence number where to start
	 * @param buf the buffer to write to
	 * @param size the size of the buffer
	 * @return the number of copied bytes
	 */
	size_t get(seq_type seqNo,void *buf,size_t size);

	/**
	 * Pulls ACKed data into <buf>. That is, it starts at the beginning and copies all data into
	 * <buf> and throws it away afterwards (rotates the window forward).
	 *
	 * @param buf the buffer to write to
	 * @param size the size of the buffer
	 * @return the number of copied bytes
	 */
	size_t pull(void *buf,size_t size);

	/**
	 * Pulls the next control-packet out of the circular buffer.
	 *
	 * @param buf the buffer to write to
	 * @param size the size of the buffer
	 * @param seqNo will be set to the used sequence number
	 * @return the number of copied bytes or 0 if there was no packet
	 */
	size_t pullctrl(void *buf,size_t size,seq_type *seqNo);

	/**
	 * Prints the state of the circular buffer to <os>.
	 *
	 * @param os the stream
	 */
	void print(std::ostream &os);

	static void unittest();

private:
	size_t getOffset(const SeqPacket &pkt,seq_type seqNo);

	size_t _max;
	size_t _current;
	std::list<SeqPacket> _packets;
	seq_type _seqStart;
	seq_type _seqAcked;
};

static inline std::ostream &operator<<(std::ostream &os,const CircularBuf &cb) {
	os << "CircularBuf[start=" << cb._seqStart << "; nextExp=" << cb.nextExp()
	   << " nextSeq=" << cb.nextSeq() << "]";
	return os;
}
