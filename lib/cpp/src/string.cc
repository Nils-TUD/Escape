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

namespace std {
	template<class InputIterator>
	string::string(InputIterator b,InputIterator e)
		: _str(NULL), _size(0), _length(0) {
		append(b,e);
	}

	template<class InputIterator>
	string& string::append(InputIterator first,InputIterator last) {
		reserve(length() + distance(first,last));
		for(; first != last; ++first)
			append(first,1);
		return *this;
	}

	template<class InputIterator>
	string& string::assign(InputIterator first,InputIterator last) {
		clear();
		return append(first,last);
	}

	template<class InputIterator>
	void string::insert(iterator p,InputIterator first,InputIterator last) {
		size_type pos1 = distance(begin(),p);
		size_type n = distance(first,last);
		reserve(_length + n);
		if(pos1 < _length)
			memmove(_str + pos1 + n,_str + pos1,(_length - pos1) * sizeof(char));
		for(; first != last; ++first)
			_str[pos1++] = *first;
		_length += n;
		_str[_length] = '\0';
	}

	template<class InputIterator>
	string& string::replace(iterator i1,iterator i2,InputIterator j1,InputIterator j2) {
		iterator pos = erase(i1,i2);
		insert(pos,j1,j2);
		return *this;
	}
}
