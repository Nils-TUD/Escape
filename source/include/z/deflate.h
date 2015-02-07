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

#include <esc/stream/istream.h>
#include <esc/stream/ostream.h>
#include <sys/common.h>
#include <z/crc32.h>
#include <z/deflatebase.h>
#include <algorithm>
#include <assert.h>

namespace z {

/**
 * Base-class for all sources, used in Deflate
 */
class DeflateSource {
public:
	virtual ~DeflateSource() {
	}

	/**
	 * Finalizes the CRC32 of the uncompressed data, if necessary.
	 *
	 * @return the CRC32 of the uncompressed data
	 */
	virtual CRC32::type crc32() = 0;

	/**
	 * @return the total number of read bytes
	 */
	virtual size_t count() const = 0;

	/**
	 * @return true if there is more data than the current block
	 */
	virtual bool more() = 0;

	/**
	 * @return the remaining data from the currently cached block
	 */
	virtual size_t cached() = 0;

	/**
	 * @param off the offset (might be negative). has to be within the current block
	 * @return the byte at current position plus <off>
	 */
	virtual uint8_t peek(ssize_t off) = 0;

	/**
	 * @return the next byte
	 */
	virtual uint8_t get() = 0;
};

/**
 * Base-class for all drains, used in Deflate
 */
class DeflateDrain {
public:
	virtual ~DeflateDrain() {
	}

	/**
	 * Writes <c> to drain
	 *
	 * @param c the character to write
	 */
	virtual void put(uint8_t c) = 0;
};

/**
 * A source implementation that reads from a stream.
 */
class StreamDeflateSource : public DeflateSource {
	static const size_t CACHE_SIZE	= 4096;

public:
	explicit StreamDeflateSource(esc::IStream &is)
		: DeflateSource(), _cache(new uint8_t[CACHE_SIZE / 2]), _next(new uint8_t[CACHE_SIZE / 2]),
		  _pos(0), _cached(0), _nextcount(0), _total(0), _checksum(0), _crc(), _is(is) {
	}
	virtual ~StreamDeflateSource() {
		delete[] _cache;
		delete[] _next;
	}

	virtual CRC32::type crc32() {
		if(_pos != _cached) {
			_checksum = _crc.update(_checksum,_cache,_pos);
			_pos = _cached;
		}
		return _checksum;
	}
	virtual size_t count() const {
		return _total;
	}
	virtual bool more() {
		load();
		return _nextcount > 0;
	}
	virtual size_t cached() {
		load();
		return _cached - _pos;
	}
	virtual uint8_t peek(ssize_t off) {
		load();
		assert(_pos + off >= 0 && _pos + off < _cached);
		return _cache[_pos + off];
	}
	virtual uint8_t get() {
		load();
		assert(_pos < _cached);
		_total++;
		return _cache[_pos++];
	}

private:
	void load() {
		if(_cached - _pos == 0) {
			bool first = _cached == 0 && _pos == 0;
			if(!first)
				_checksum = _crc.update(_checksum,_cache,_pos);
			_cached = _is.read(_cache,CACHE_SIZE / 2);
			_pos = 0;
			if(first)
				_nextcount = _is.read(_next,CACHE_SIZE / 2);
			else {
				std::swap(_cache,_next);
				std::swap(_cached,_nextcount);
			}
		}
	}

	uint8_t *_cache;
	uint8_t *_next;
	size_t _pos;
	size_t _cached;
	size_t _nextcount;
	size_t _total;
	CRC32::type _checksum;
	CRC32 _crc;
	esc::IStream &_is;
};

/**
 * A drain implementation that writes to a stream
 */
class StreamDeflateDrain : public DeflateDrain {
public:
	explicit StreamDeflateDrain(esc::OStream &os)
		: DeflateDrain(), _os(os) {
	}

	virtual void put(uint8_t c) {
		_os.write(c);
	}

private:
	esc::OStream &_os;
};

/**
 * The encoder part of the deflate compression algorithm.
 */
class Deflate : public DeflateBase {
	struct Tree {
		unsigned int litbase[4];
		unsigned int length[4];
		unsigned int codebase[4];
	};

	struct Data {
		DeflateSource *source;
		unsigned int tag;
		unsigned int bitcount;

		DeflateDrain *drain;
	};

	enum {
		OK		= 0,
		FAILED	= -1
	};

public:
	enum Level {
		NONE	= 0,
		FIXED	= 1,
		DYN		= 2
	};

	/**
	 * Constructor
	 */
	explicit Deflate();

	/**
	 * Compresses the data in <source> into <drain>.
	 *
	 * @param drain the destination
	 * @param source the source
	 * @param compr the compression level
	 * @return 0 on success or -1 on error
	 */
	int compress(DeflateDrain *drain,DeflateSource *source,int compr);

private:
	void flush(Data *d);
	void writebit(Data *d,int bit);
	void write_bits(Data *d,unsigned int bits,int num);
	void write_bits_reverse(Data *d,unsigned int bits,unsigned int num);
	void write_symbol(Data *d,Tree *t,unsigned int sym);
	void write_int(Data *d,Tree *t,unsigned short *base,unsigned char *bits,unsigned int value,int add);
	void write_symbol_chain(Data *d,Tree *lt,Tree *dt,unsigned int len,unsigned int dist);

	void deflate_fixed_block(Data *d);
	void deflate_uncompressed_block(Data *d);

	Tree sltree;
	Tree sdtree;
};

}
