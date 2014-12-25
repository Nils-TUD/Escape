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
#include <iostream>

namespace z {

/**
 * The header of a gzip file
 */
struct GZipHeader {
	static const size_t MAX_NAME_LEN	= 64;
	static const size_t MAX_COMMENT_LEN	= 256;

	enum Flags {
		FTEXT		= 1 << 0,
		FHCRC		= 1 << 1,
		FEXTRA		= 1 << 2,
		FNAME		= 1 << 3,
		FCOMMENT	= 1 << 4,
	};

	enum Methods {
		MDEFLATE	= 8
	};

	/**
	 * Reads the header from given file
	 *
	 * @param f the file
	 * @return the GZip header
	 */
	static GZipHeader read(FILE *f);

	explicit GZipHeader()
		: id1(), id2(), method(), flags(), mtime(), xflags(), os(), filename(), comment() {
	}
	GZipHeader(const GZipHeader&) = delete;
	GZipHeader &operator=(const GZipHeader&) = delete;
	GZipHeader(GZipHeader &&h)
		: id1(h.id1), id2(h.id2), method(h.method), flags(h.flags), mtime(h.mtime), xflags(h.xflags),
		  os(h.os), filename(h.filename), comment(h.comment) {
		h.filename = h.comment = NULL;
  	}
	~GZipHeader() {
		delete[] filename;
		delete[] comment;
	}

	/**
	 * @return true if the id is valid
	 */
	bool isGZip() const {
		return id1 == 0x1f && id2 == 0x8b;
	}

	/**
	 * Writes the information in <h> in a human readable form to <os>.
	 *
	 * @param os the output-stream to write to
	 * @param h the header
	 * @return os
	 */
	friend std::ostream &operator<<(std::ostream &os,const GZipHeader &h);

	uint8_t id1;
	uint8_t id2;
	uint8_t method;
	uint8_t flags;
	uint32_t mtime;
	uint8_t xflags;
	uint8_t os;
	char *filename;
	char *comment;
} A_PACKED;

}
