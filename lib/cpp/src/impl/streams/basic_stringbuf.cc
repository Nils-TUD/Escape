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
	template<class charT,class traits>
	basic_stringbuf<charT,traits>::basic_stringbuf(ios_base::openmode which)
		: basic_streambuf<charT,traits>(), _pos(0), _mode(which), _str(basic_string<charT>()) {
	}
	template<class charT,class traits>
	basic_stringbuf<charT,traits>::basic_stringbuf(const basic_string<charT>& str,ios_base::openmode which)
		: basic_streambuf<charT,traits>(), _pos(0), _mode(which), _str(str) {
		if(_mode & ios_base::trunc)
			_str.clear();
		if(_mode & ios_base::ate)
			_pos = _str.length();
	}
	template<class charT,class traits>
	basic_stringbuf<charT,traits>::~basic_stringbuf() {
	}

	template<class charT,class traits>
	basic_string<charT> basic_stringbuf<charT,traits>::str() const {
		return _str;
	}
	template<class charT,class traits>
	void basic_stringbuf<charT,traits>::str(const basic_string<charT>& s) {
		// reset position
		_pos = 0;
		_str = s;
	}

	template<class charT,class traits>
	typename basic_stringbuf<charT,traits>::pos_type basic_stringbuf<charT,traits>::available() const {
		return _str.length() - _pos;
	}

	template<class charT,class traits>
	typename basic_stringbuf<charT,traits>::char_type basic_stringbuf<charT,traits>::peek() const {
		if(!(_mode & ios_base::in))
			throw bad_state(string("No read-permission"));
		if(_pos >= _str.length())
			throw eof_reached();
		return _str[_pos];
	}
	template<class charT,class traits>
	typename basic_stringbuf<charT,traits>::char_type basic_stringbuf<charT,traits>::get() {
		if(!(_mode & ios_base::in))
			throw bad_state(string("No read-permission"));
		if(_pos >= _str.length())
			throw eof_reached();
		return _str[_pos++];
	}
	template<class charT,class traits>
	void basic_stringbuf<charT,traits>::unget() {
		if(!(_mode & ios_base::in) || _pos == 0)
			throw bad_state(string("No read-permission or unable to move back"));
		_pos--;
	}

	template<class charT,class traits>
	void basic_stringbuf<charT,traits>::put(char_type c) {
		if(!(_mode & ios_base::out))
			throw bad_state(string("No write-permission"));
		if(_mode & ios_base::app)
			_pos = _str.length();
		_str.insert(_pos++,&c,1);
	}

	template<class charT,class traits>
	void basic_stringbuf<charT,traits>::flush() {
	}
}
