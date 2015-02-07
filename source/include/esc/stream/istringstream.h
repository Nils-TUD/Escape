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
#include <string>
#include <string.h>

namespace esc {

/**
 * Input-stream that reads from a string
 */
class IStringStream : public IStream {
public:
	/**
	 * Reads a value of type <T> from the given string
	 *
	 * @param str the string
	 * @return the read value
	 */
	template<typename T>
	static T read_from(const std::string &str) {
		IStringStream is(const_cast<std::string&>(str));
		T t;
		is >> t;
		return t;
	}

	/**
	 * Constructor
	 *
	 * @param str the string (not copied and not changed, but non-const to prevent that someone
	 *  accidently passes in a temporary)
	 */
	explicit IStringStream(std::string &str)
		: IStream(), _str(str.c_str()), _len(str.length()), _pos() {
	}
	/**
	 * Constructor
	 *
	 * @param str the string to read from
	 * @param len the length of the string (-1 to use strlen())
	 */
	explicit IStringStream(const char *str,size_t len = -1)
		: IStream(), _str(str), _len(len != (size_t)-1 ? len : strlen(str)), _pos() {
	}

	virtual char read() override {
		if(_pos < _len)
			return _str[_pos++];
		_state |= FL_EOF;
		return '\0';
	}
	virtual size_t read(void *dst,size_t count) override {
		if(_pos + count < _pos || _pos + count > _len)
			count = _len - _pos;
		memcpy(dst,_str + _pos,count);
		_pos += count;
		if(_pos == _len)
			_state |= FL_EOF;
		return count;
	}
	virtual bool putback(char c) override {
		if(_pos == 0 || _str[_pos - 1] != c)
			return false;
		_pos--;
		return true;
	}

private:
	const char *_str;
	size_t _len;
	size_t _pos;
};

}
