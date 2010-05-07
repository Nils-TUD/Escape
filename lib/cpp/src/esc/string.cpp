/**
 * $Id: string.cpp 479 2010-02-07 11:27:21Z nasmussen $
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
	String::String(u32 size)
		: _str(new char[size]), _length(0), _size(size) {
	}

	String::String(const String &s)
		: _str(new char[s._size]), _length(s._length), _size(s._size) {
		strcpy(_str,s._str);
	}

	String::String(const char *s)
		: _str(NULL), _length(0), _size(0) {
		if(s != NULL) {
			u32 len = strlen(s);
			_str = new char[len + 1];
			strcpy(_str,s);
			_length = len;
			_size = len + 1;
		}
		else {
			_str = new char[initSize];
			_length = 0;
			_size = initSize;
		}
	}

	String::~String() {
		delete[] _str;
	}

	void String::reserve(u32 size) {
		increaseSize(size);
	}

	u32 String::find(char c,u32 offset) const {
		char *start = _str + offset;
		char *res = strchr(start,c);
		if(res)
			return res - start;
		return npos;
	}

	u32 String::find(const char *sub,u32 offset) const {
		if(sub == NULL)
			return npos;
		char *start = _str + offset;
		char *res = strstr(start,sub);
		if(res)
			return res - start;
		return npos;
	}

	void String::insert(u32 offset,char c) {
		if(offset == _length)
			operator +=(c);
		else if(offset < _length) {
			increaseSize(_length + 1);
			memmove(_str + offset + 1,_str + offset,1 + _length - offset);
			_str[offset] = c;
			_length++;
		}
	}

	void String::insert(u32 offset,const String &s) {
		if(offset == _length)
			operator +=(s);
		else if(offset < _length) {
			increaseSize(_length + s._length);
			memmove(_str + offset + s._length,_str + offset,_length - offset);
			memcpy(_str + offset,s._str,s._length);
			_length += s._length;
		}
	}

	void String::erase(u32 offset,u32 count) {
		if(offset < _length) {
			count = MIN(_length - offset,count);
			if(count == 0)
				return;
			// something behind?
			if(_length - offset - count > 0)
				memmove(_str + offset,_str + offset + count,_length - offset - count);
			_length -= count;
			_str[_length] = '\0';
		}
	}

	String &String::operator=(const String &s) {
		_str = new char[s._size];
		strcpy(_str,s._str);
		_length = s._length;
		_size = s._size;
		return *this;
	}

	String &String::operator+=(char c) {
		increaseSize(_length + 1);
		_str[_length++] = c;
		_str[_length] = '\0';
		return *this;
	}

	String &String::operator+=(const String &s) {
		increaseSize(_length + s._length);
		strcpy(_str + _length,s._str);
		_length += s._length;
		return *this;
	}

	void String::increaseSize(u32 min) {
		char *cpy;
		// we need a null-termination
		if(_size < min + 1) {
			_size = MAX(min + 1,_size * 2);
			cpy = new char[_size];
			strcpy(cpy,_str);
			delete[] _str;
			_str = cpy;
		}
	}

	Stream &operator<<(Stream &s,String &str) {
		s << str._str;
		return s;
	}
}
