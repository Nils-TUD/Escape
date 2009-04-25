/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <esc/common.h>
#include <esc/string.h>
#include <esc/stream.h>
#include <string.h>

namespace esc {
	u32 String::find(char c,u32 offset) const {
		char *start = _str + offset;
		char *res = strchr(start,c);
		if(res)
			return res - start;
		return npos;
	}

	u32 String::find(const char *sub,u32 offset) const {
		char *start = _str + offset;
		char *res = strstr(start,sub);
		if(res)
			return res - start;
		return npos;
	}

	String &String::operator+=(char c) {
		increaseSize(_length + 1);
		_str[_length++] = c;
		_str[_length] = '\0';
		return *this;
	}

	String &String::operator+=(const char *s) {
		u32 len = ::strlen(s);
		increaseSize(_length + len);
		::strcpy(_str + _length,s);
		_length += len;
		return *this;
	}

	String &String::operator+=(const String &s) {
		increaseSize(_length + s._length);
		::strcpy(_str + _length,s._str);
		_length += s._length;
		return *this;
	}

	bool String::operator==(const String &s) const {
		return _length == s._length && strcmp(_str,s._str) == 0;
	}

	bool String::operator!=(const String &s) const {
		return _length != s._length || strcmp(_str,s._str) != 0;
	}

	void String::increaseSize(u32 min) {
		char *cpy;
		// we need a null-termination
		if(_size < min + 1) {
			_size = MAX(min,_size * 2);
			cpy = new char[_size];
			if(_str) {
				::strcpy(cpy,_str);
				delete _str;
			}
			_str = cpy;
		}
	}

	Stream &operator<<(Stream &s,String &str) {
		s << str._str;
		return s;
	}
}
