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
#include <z/crc32.h>
#include <z/deflatebase.h>
#include <stdio.h>
#include <assert.h>
#include <algorithm>

namespace z {

/**
 * Base-class for all sources, used in Inflate
 */
class InflateSource {
public:
	virtual ~InflateSource() {
	}

	/**
	 * @return the next byte
	 */
	virtual uint8_t get() = 0;
};

/**
 * Base-class for all drains, used in Inflate
 */
class InflateDrain {
public:
	virtual ~InflateDrain() {
	}

	/**
	 * Finalizes the CRC32 of the uncompressed data, if necessary.
	 *
	 * @return the CRC32 of the uncompressed data
	 */
	virtual CRC32::type crc32() = 0;

	/**
	 * Should return the previously written character, written <off> bytes ago.
	 *
	 * @param off the offset (at most 32*1024)
	 * @return the character written <off> bytes ago
	 */
	virtual uint8_t get(size_t off) = 0;

	/**
	 * Writes <c> to drain
	 *
	 * @param c the character to write
	 */
	virtual void put(uint8_t c) = 0;
};

/**
 * A source implementation that reads from a FILE.
 */
class FileInflateSource : public InflateSource {
public:
	explicit FileInflateSource(FILE *file)
		: InflateSource(), _file(file) {
	}

	virtual uint8_t get() {
		return fgetc(_file);
	}

private:
	FILE *_file;
};

/**
 * A drain implementation that writes to a FILE
 */
class FileInflateDrain : public InflateDrain {
public:
	static const size_t BUF_SIZE	= 32 * 1024;	// has to be at least 32K

	explicit FileInflateDrain(FILE *file)
		: InflateDrain(), _crc(), _checksum(0), _file(file), _buf(new uint8_t[BUF_SIZE]), _wpos() {
	}
	virtual ~FileInflateDrain() {
		delete[] _buf;
	}

	virtual CRC32::type crc32() {
		if(_wpos > 0) {
			_checksum = _crc.update(_checksum,_buf,_wpos);
			_wpos = 0;
		}
		return _checksum;
	}

	virtual uint8_t get(size_t off) {
		assert(off < BUF_SIZE);
		return _buf[(_wpos - off) % BUF_SIZE];
	}
	virtual void put(uint8_t c) {
		fputc(c,_file);
		_buf[_wpos] = c;
		_wpos = (_wpos + 1) % BUF_SIZE;
		if(_wpos == 0)
			_checksum = _crc.update(_checksum,_buf,BUF_SIZE);
	}

private:
	CRC32 _crc;
	CRC32::type _checksum;
	FILE *_file;
	uint8_t *_buf;
	size_t _wpos;
};

class MemInflateSource : public InflateSource {
public:
	explicit MemInflateSource(void *buffer,size_t size)
		: _buffer(reinterpret_cast<uint8_t*>(buffer)), _size(size), _pos() {
	}

	virtual uint8_t get() {
		if(_pos >= _size)
			return 0;
		return _buffer[_pos++];
	}

private:
	uint8_t *_buffer;
	size_t _size;
	size_t _pos;
};

class MemInflateDrain : public InflateDrain {
public:
	explicit MemInflateDrain(void *buffer,size_t size)
		: _buffer(reinterpret_cast<uint8_t*>(buffer)), _size(size), _pos() {
	}

	virtual CRC32::type crc32() {
		CRC32 crc;
		return crc.get(_buffer,_pos);
	}

	virtual uint8_t get(size_t off) {
		assert(off <= _pos);
		return _buffer[_pos - off];
	}

	virtual void put(uint8_t c) {
		if(_pos < _size)
			_buffer[_pos++] = c;
	}

private:
	uint8_t *_buffer;
	size_t _size;
	size_t _pos;
};

class Inflate : public DeflateBase {
	struct Tree {
		unsigned short table[16];  /* table of code length counts */
		unsigned short trans[288]; /* code -> symbol translation table */
	};

	struct Data {
		InflateSource *source;
		unsigned int tag;
		unsigned int bitcount;

		InflateDrain *drain;

		Tree ltree; /* dynamic length/symbol tree */
		Tree dtree; /* dynamic distance tree */
	};

	enum {
		OK		= 0,
		FAILED	= -1
	};

public:
	/**
	 * Constructor
	 */
	explicit Inflate();

	/**
	 * Uncompresses the data in <source> into <drain>.
	 *
	 * @param drain the destination
	 * @param source the source
	 * @return 0 on success or -1 on error
	 */
	int uncompress(InflateDrain *drain,InflateSource *source);

private:
	void build_fixed_trees(Tree *lt,Tree *dt);
	void build_tree(Tree *t,const unsigned char *lengths,unsigned int num);

	void flush(Data *d);
	void writebit(Data *d,int bit);
	void write_bits(Data *d,unsigned int bits,int num);

	int getbit(Data *d);
	unsigned int read_bits(Data *d,int num,int base);
	int decode_symbol(Data *d,Tree *t);
	void decode_trees(Data *d,Tree *lt,Tree *dt);

	int deflate_uncompressed_block(Data *d);

	int inflate_block_data(Data *d,Tree *lt,Tree *dt);
	int inflate_uncompressed_block(Data *d);
	int inflate_fixed_block(Data *d);
	int inflate_dynamic_block(Data *d);

	Tree sltree; /* fixed length/symbol tree */
	Tree sdtree; /* fixed distance tree */

	/* special ordering of code length codes */
	static const unsigned char clcidx[];
};

}
