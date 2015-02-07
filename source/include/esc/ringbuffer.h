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
#include <sys/common.h>

namespace esc {

template<class T>
class RingBuffer;

template<class T>
static OStream &operator<<(OStream &os,const RingBuffer<T> &rb);

enum {
	RB_DEFAULT		= 0x0,		/* error if ring-buffer full */
	RB_OVERWRITE	= 0x1,		/* overwrite if full */
};

template<class T>
class RingBuffer {
	template<class T2>
	friend OStream &operator<<(OStream &os,const RingBuffer<T2> &rb);

public:
	/**
	 * Creates a new ring-buffer with given maximum element-count.
	 *
	 * @param max the max. number of elements in the ring
	 * @param flags RB_DEFAULT or RB_OVERWRITE
	 */
	explicit RingBuffer(size_t max,uint flags = RB_DEFAULT) :
		_count(), _readPos(), _writePos(), _eMax(max), _flags(flags), _data(new T[max]) {
	}

	/**
	 * Destroys this ring-buffer
	 */
	~RingBuffer() {
		delete[] _data;
	}

	/**
	 * @return the number of elements that can be read
	 */
	size_t length() const {
		return _count;
	}

	/**
	 * Writes the given element into the ring-buffer. If F_OVERWRITE is set for the ring-buffer
	 * and the buffer is full the first unread element will simply be overwritten. If F_OVERWRITE
	 * is not set, this will result in an error.
	 *
	 * @param e the element
	 * @return true if successfull
	 */
	bool write(const T &e) {
		bool maxReached = _count >= _eMax;
		if(!(_flags & RB_OVERWRITE) && maxReached)
			return false;
		_data[_writePos] = e;
		_writePos = (_writePos + 1) % _eMax;
		if((_flags & RB_OVERWRITE) && maxReached)
			_readPos = (_readPos + 1) % _eMax;
		else
			_count++;
		return true;
	}

	/**
	 * Writes <n> elements from <e> into the ring-buffer.
	 *
	 * @param e the address of the elements
	 * @param n the number of elements to write
	 * @return the number of written elements
	 */
	size_t writen(const T *e,size_t n) {
		size_t orgn = n;
		for(; n > 0; ++e, --n) {
			if(!write(*e))
				break;
		}
		return orgn - n;
	}

	/**
	 * Reads the next element from the ring-buffer. If there is no element, false is returned.
	 *
	 * @param r the ring-buffer
	 * @param e the address where to copy the element
	 * @return true if successfull
	 */
	bool read(T *e) {
		if(_count == 0)
			return false;
		*e = _data[_readPos];
		_readPos = (_readPos + 1) % _eMax;
		_count--;
		return true;
	}

	/**
	 * Reads <n> elements to <e> from the ring-buffer. Assumes that T is a POD type.
	 *
	 * @param e the address where to copy the elements
	 * @param n the number of elements to read
	 * @return the number of read elements
	 */
	size_t readn(T *e,size_t n) {
		size_t orgn = n;
		while(n > 0 && _count > 0) {
			size_t count = MIN(_eMax - _readPos,n);
			count = MIN(_count,count);
			memcpy(e,_data + _readPos,count * sizeof(T));
			n -= count;
			e += count;
			_count -= count;
			_readPos = (_readPos + count) % _eMax;
		}
		return orgn - n;
	}

	/**
	 * Moves the next <n> elements from <src> to <this>.
	 *
	 * @param src the source-ring-buffer
	 * @param n the number of elements to move
	 * @return the number of moved elements
	 */
	size_t move(RingBuffer &src,size_t n) {
		size_t count,c = 0;
		while(n > 0 && src._count > 0) {
			count = MIN(src._eMax - src._readPos,n);
			count = MIN(src._count,count);
			count = writen(src._data + src._readPos,count);
			n -= count;
			c += count;
			src._count -= count;
			src._readPos = (src._readPos + count) % src._eMax;
		}
		return c;
	}

private:
	size_t _count;		/* current number of elements that can be read */
	size_t _readPos;	/* current read-pos */
	size_t _writePos;	/* current write-pos */
	size_t _eMax;		/* max number of entries in <data> */
	uint _flags;
	T *_data;
};

template<class T>
static inline OStream &operator<<(OStream &os,const RingBuffer<T> &rb) {
	os << "RingBuffer [cnt=%zu, rpos=%zu, wpos=%zu, emax=%zu]:",rb._count,rb._readPos,
			rb._writePos,rb._eMax,rb._eSize << "\n";
	size_t c,i = rb._readPos;
	for(c = 0, i = rb._readPos; c < rb._count; c++, i = (i + 1) % rb._eMax)
		os << "  " << rb._data[i] << "\n";
}

}
