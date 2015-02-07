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
#include <string>

namespace esc {

/**
 * Output-stream that writes to a string
 */
class OStringStream : public OStream {
public:
	explicit OStringStream() : OStream(), _str() {
	}

	/**
	 * @return the string
	 */
	const std::string &str() const {
		return _str;
	}

	virtual void write(char c) override {
		_str.append(1,c);
	}
	virtual size_t write(const void *src,size_t count) override {
		_str.append(reinterpret_cast<const char*>(src),count);
		return count;
	}

private:
	std::string _str;
};

}
