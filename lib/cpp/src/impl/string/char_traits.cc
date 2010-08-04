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

namespace esc {
	template<typename CharT>
	void char_traits<CharT>::assign(char_type& c1,const char_type& c2) {
		c1 = c2;
	}
	template<typename CharT>
	bool char_traits<CharT>::eq(const char_type& c1,const char_type& c2) {
		return c1 == c2;
	}
	template<typename CharT>
	bool char_traits<CharT>::lt(const char_type& c1,const char_type& c2) {
		return c1 < c2;
	}

	template<typename CharT>
	int char_traits<CharT>::compare(const char_type* s1,const char_type* s2,size_t n) {
		for(size_t i = 0; i < n; ++i) {
			if(lt(s1[i],s2[i]))
				return -1;
			else if(lt(s2[i],s1[i]))
				return 1;
		}
		return 0;
	}
	template<typename CharT>
	size_t char_traits<CharT>::length(const char_type* s) {
		size_t i = 0;
		while(!eq(s[i],char_type()))
			++i;
		return i;
	}
	template<typename CharT>
	const typename char_traits<CharT>::char_type* char_traits<CharT>::find(
			const char_type* s,size_t n,const char_type& a) {
		for(size_t i = 0; i < n; ++i) {
			if(eq(s[i],a))
				return s + i;
		}
		return 0;
	}
	template<typename CharT>
	typename char_traits<CharT>::char_type* char_traits<CharT>::move(
			char_type* s1,const char_type* s2,size_t n) {
		return static_cast<CharT*>(memmove(s1,s2,n * sizeof(char_type)));
	}
	template<typename CharT>
	typename char_traits<CharT>::char_type* char_traits<CharT>::copy(
			char_type* s1,const char_type* s2,size_t n) {
		esc::copy(s2,s2 + n,s1);
		return s1;
	}
	template<typename CharT>
	typename char_traits<CharT>::char_type* char_traits<CharT>::assign(
			char_type* s,size_t n,char_type a) {
		esc::fill_n(s,n,a);
		return s;
	}

	template<typename CharT>
	typename char_traits<CharT>::char_type char_traits<CharT>::to_char_type(const int_type& c) {
		return static_cast<char_type> (c);
	}
	template<typename CharT>
	typename char_traits<CharT>::int_type char_traits<CharT>::to_int_type(const char_type& c) {
		return static_cast<int_type> (c);
	}
	template<typename CharT>
	bool char_traits<CharT>::eq_int_type(const int_type& c1,const int_type& c2) {
		return c1 == c2;
	}

	template<typename CharT>
	typename char_traits<CharT>::int_type char_traits<CharT>::eof() {
		return static_cast<int_type>(EOF);
	}
	template<typename CharT>
	typename char_traits<CharT>::int_type char_traits<CharT>::not_eof(const int_type& c) {
		return !eq_int_type(c,eof()) ? c : to_int_type(char_type());
	}
}
