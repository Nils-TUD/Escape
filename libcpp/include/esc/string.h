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
		static const u32 initSize = 8;
	public:
		static const u32 npos = -1;

	public:
		String();
		String(const String &s);
		String(const char *s);
		~String();

		inline u32 length() const {
			return _length;
		};

		u32 find(char c,u32 offset = 0) const;
		u32 find(const char *sub,u32 offset = 0) const;

		void insert(u32 offset,char c);
		void insert(u32 offset,const char *s);
		void insert(u32 offset,const String &s);
		inline void append(char c) {
			operator +=(c);
		};
		inline void append(const char *s) {
			operator +=(s);
		};
		inline void append(const String &s) {
			operator +=(s);
		};
		inline void erase(u32 offset) {
			erase(offset,_length - offset);
		};
		void erase(u32 offset,u32 count);

		String &operator=(const String &s);
		String &operator+=(char c);
		String &operator+=(const char *s);
		String &operator+=(const String &s);

		inline bool operator==(const String &s) const {
			return _length == s._length && strcmp(_str,s._str) == 0;
		};
		inline bool operator!=(const String &s) const {
			return _length != s._length || strcmp(_str,s._str) != 0;
		};
		inline char operator[](u32 index) const {
			return _str[index];
		};

	private:
		void increaseSize(u32 min);

		char *_str;
		u32 _length;
		u32 _size;
	};

	Stream &operator<<(Stream &s,String &str);
}

#endif /* CPP_STRING_H_ */
