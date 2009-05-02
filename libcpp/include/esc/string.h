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

#ifndef CPP_STRING_H_
#define CPP_STRING_H_

#include <esc/common.h>
#include <esc/stream.h>
#include <string.h>

namespace esc {
	class String {
		friend Stream &operator<<(Stream &s,String &str);

	private:
		static const u32 initLength = 16;
	public:
		static const u32 npos = -1;

	public:
		String() : _str(NULL), _length(0), _size(0) {};
		String(const String &s) {
			_str = new char[s._size];
			strcpy(_str,s._str);
			_length = s._length;
			_size = s._size;
		};
		String(const char *s) {
			u32 len = strlen(s);
			_str = new char[len + 1];
			strcpy(_str,s);
			_length = len;
			_size = len + 1;
		};

		~String() {
			delete _str;
		};

		inline u32 length() const {
			return _length;
		};

		u32 find(char c,u32 offset = 0) const;
		u32 find(const char *sub,u32 offset = 0) const;

		String &operator+=(char c);
		String &operator+=(const char *s);
		String &operator+=(const String &s);
		bool operator==(const String &s) const;
		bool operator!=(const String &s) const;
		char operator[](u32 index) const;

	private:
		void increaseSize(u32 min);

		char *_str;
		u32 _length;
		u32 _size;
	};

	Stream &operator<<(Stream &s,String &str);
}

#endif /* CPP_STRING_H_ */
