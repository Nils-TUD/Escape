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

#include <common.h>
#include <mem/cache.h>
#include <ostream.h>

class OStringStream : public OStream {
	static const size_t INIT_SIZE	= 16;

public:
	explicit OStringStream(char *str,size_t size)
		: OStream(), dynamic(false), str(str), size(size), len(0) {
	}
	explicit OStringStream()
		: OStream(), dynamic(true), str(static_cast<char*>(Cache::alloc(INIT_SIZE))),
		  size(INIT_SIZE), len(0) {
	}
	virtual ~OStringStream() {
		if(dynamic)
			Cache::free(str);
	}

	const char *getString() const {
		return str;
	}
	size_t getLength() const {
		return len;
	}
	char *keepString() {
		dynamic = false;
		return str;
	}

	virtual void writec(char c);

private:

	bool dynamic;
	char *str;
	size_t size;
	size_t len;
};
