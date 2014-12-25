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
#include <stdio.h>
#include <assert.h>

namespace z {

/**
 * Base-class for all sources, used in Deflate
 */
class Source {
public:
	virtual ~Source() {
	}

	/**
	 * @return the next byte
	 */
	virtual char get() = 0;
};

/**
 * Base-class for all drains, used in Deflate
 */
class Drain {
public:
	virtual ~Drain() {
	}

	/**
	 * Should return the previously written character, written <off> bytes ago.
	 *
	 * @param off the offset (at most 32*1024)
	 * @return the character written <off> bytes ago
	 */
	virtual char get(size_t off) = 0;

	/**
	 * Writes <c> to drain
	 *
	 * @param c the character to write
	 */
	virtual void put(char c) = 0;
};

/**
 * A source implementation that reads from a FILE.
 */
class FileSource : public Source {
public:
	explicit FileSource(FILE *file) : Source(), _file(file) {
	}

	virtual char get() {
		return fgetc(_file);
	}

private:
	FILE *_file;
};

/**
 * A drain implementation that writes to a FILE
 */
class FileDrain : public Drain {
public:
	static const size_t BUF_SIZE	= 32 * 1024;	// has to be at least 32K

	explicit FileDrain(FILE *file)
		: Drain(), _crc(), _checksum(0), _file(file), _buf(new char[BUF_SIZE]), _wpos() {
	}
	~FileDrain() {
		delete[] _buf;
	}

	/**
	 * Finalizes the CRC32 of the uncompressed data, if necessary.
	 *
	 * @return the CRC32 of the uncompressed data
	 */
	ulong crc32() {
		if(_wpos > 0) {
			_checksum = _crc.update(_checksum,_buf,_wpos);
			_wpos = 0;
		}
		return _checksum;
	}

	virtual char get(size_t off) {
		assert(off < BUF_SIZE);
		return _buf[(_wpos - off) % BUF_SIZE];
	}
	virtual void put(char c) {
		fputc(c,_file);
		_buf[_wpos] = c;
		_wpos = (_wpos + 1) % BUF_SIZE;
		if(_wpos == 0)
			_checksum = _crc.update(_checksum,_buf,BUF_SIZE);
	}

private:
	CRC32 _crc;
	ulong _checksum;
	FILE *_file;
	char *_buf;
	size_t _wpos;
};

/**
 * Implements the deflate compression algorithm. At the moment, only uncompress is implemented.
 */
class Deflate {
	struct TINF_TREE {
		unsigned short table[16];  /* table of code length counts */
		unsigned short trans[288]; /* code -> symbol translation table */
	};

	struct TINF_DATA {
		Source *source;
		unsigned int tag;
		unsigned int bitcount;

		Drain *drain;

		TINF_TREE ltree; /* dynamic length/symbol tree */
		TINF_TREE dtree; /* dynamic distance tree */
	};

	enum {
		TINF_OK			= 0,
		TINF_DATA_ERROR	= -1
	};

public:
	/**
	 * Constructor
	 */
	explicit Deflate();

	/**
	 * Uncompresses the data in <source> into <drain>.
	 *
	 * @param drain the destination
	 * @param source the source
	 * @return 0 on success or -1 on error
	 */
	int uncompress(Drain *drain,Source *source);

private:
	void tinf_build_bits_base(unsigned char *bits,unsigned short *base,int delta,int first);
	void tinf_build_fixed_trees(TINF_TREE *lt,TINF_TREE *dt);
	void tinf_build_tree(TINF_TREE *t,const unsigned char *lengths,unsigned int num);
	int tinf_getbit(TINF_DATA *d);
	unsigned int tinf_read_bits(TINF_DATA *d,int num,int base);
	int tinf_decode_symbol(TINF_DATA *d,TINF_TREE *t);
	void tinf_decode_trees(TINF_DATA *d,TINF_TREE *lt,TINF_TREE *dt);
	int tinf_inflate_block_data(TINF_DATA *d,TINF_TREE *lt,TINF_TREE *dt);
	int tinf_inflate_uncompressed_block(TINF_DATA *d);
	int tinf_inflate_fixed_block(TINF_DATA *d);
	int tinf_inflate_dynamic_block(TINF_DATA *d);

	TINF_TREE sltree; /* fixed length/symbol tree */
	TINF_TREE sdtree; /* fixed distance tree */

	/* extra bits and base tables for length codes */
	unsigned char length_bits[30];
	unsigned short length_base[30];

	/* extra bits and base tables for distance codes */
	unsigned char dist_bits[30];
	unsigned short dist_base[30];

	/* special ordering of code length codes */
	static const unsigned char clcidx[];
};

}
