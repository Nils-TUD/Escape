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

#ifndef CHAR_TRAITS_H_
#define CHAR_TRAITS_H_

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

namespace std {
	template<typename CharT>
	struct char_traits {
		typedef CharT char_type;
		typedef unsigned long int_type;
		//typedef streampos pos_type;
		//typedef streamoff off_type;
		typedef signed long pos_type;
		typedef signed long off_type;
		// TODO
		typedef unsigned long state_type;

		static void assign(char_type& c1,const char_type& c2);
		static bool eq(const char_type& c1,const char_type& c2);
		static bool lt(const char_type& c1,const char_type& c2);

		static int compare(const char_type* s1,const char_type* s2,size_t n);
		static size_t length(const char_type* s);
		static const char_type* find(const char_type* s,size_t n,const char_type& a);
		static char_type* move(char_type* s1,const char_type* s2,size_t n);
		static char_type* copy(char_type* s1,const char_type* s2,size_t n);
		static char_type* assign(char_type* s,size_t n,char_type a);

		static char_type to_char_type(const int_type& c);
		static int_type to_int_type(const char_type& c);
		static bool eq_int_type(const int_type& c1,const int_type& c2);

		static int_type eof();
		static int_type not_eof(const int_type& c);
	};

	template<>
	struct char_traits<char> {
		typedef char char_type;
		typedef int int_type;
		//typedef streampos pos_type;
		//typedef streamoff off_type;
		typedef signed long pos_type;
		typedef signed long off_type;
		// TODO
		typedef unsigned long state_type;

		static void assign(char_type& c1,const char_type& c2) {
			c1 = c2;
		}
		static bool eq(const char_type& c1,const char_type& c2) {
			return c1 == c2;
		}
		static bool lt(const char_type& c1,const char_type& c2) {
			return c1 < c2;
		}

		static int compare(const char_type* s1,const char_type* s2,size_t n) {
			return memcmp(s1,s2,n);
		}
		static size_t length(const char_type* s) {
			return strlen(s);
		}
		static const char_type* find(const char_type* s,size_t n,const char_type& a) {
			return static_cast<char_type*>(memchr(s,a,n));
		}
		static char_type* move(char_type* s1,const char_type* s2,size_t n) {
			return static_cast<char_type*>(memmove(s1,s2,n));
		}
		static char_type* copy(char_type* s1,const char_type* s2,size_t n) {
			return static_cast<char_type*>(memcpy(s1,s2,n));
		}
		static char_type* assign(char_type* s,size_t n,char_type a) {
			memset(s,a,n);
			return s;
		}

		static char_type to_char_type(const int_type& c) {
			return static_cast<char_type>(c);
		}
		static int_type to_int_type(const char_type& c) {
			return static_cast<int_type>(static_cast<unsigned char>(c));
		}
		static bool eq_int_type(const int_type& c1,const int_type& c2) {
			return c1 == c2;
		}

		static int_type eof() {
			return static_cast<int_type>(EOF);
		}
		static int_type not_eof(const int_type& c) {
			return (c == eof()) ? 0 : c;
		}
	};
}

#include "../../../../lib/cpp/impl/string/char_traits.cc"

#endif /* CHAR_TRAITS_H_ */
