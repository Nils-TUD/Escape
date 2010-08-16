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

#include <impl/streams/stringbuf.h>
#include <stdio.h>

namespace std {
	stringbuf::stringbuf(ios_base::openmode which)
		: streambuf(), _pos(0), _mode(which), _str(string()) {
	}
	stringbuf::stringbuf(const string& s,ios_base::openmode which)
		: streambuf(), _pos(0), _mode(which), _str(s) {
		if(_mode & ios_base::trunc)
			_str.clear();
		if(_mode & ios_base::ate)
			_pos = _str.length();
	}
	stringbuf::~stringbuf() {
	}

	string stringbuf::str() const {
		return _str;
	}
	void stringbuf::str(const string& s) {
		// reset position
		_pos = 0;
		_str = s;
	}

	stringbuf::pos_type stringbuf::available() const {
		return _str.length() - _pos;
	}

	stringbuf::char_type stringbuf::peek() const {
		if(!(_mode & ios_base::in))
			throw bad_state(string("No read-permission"));
		if(_pos >= _str.length())
			return EOF;
		return _str[_pos];
	}
	stringbuf::char_type stringbuf::get() {
		if(!(_mode & ios_base::in))
			throw bad_state(string("No read-permission"));
		if(_pos >= _str.length())
			return EOF;
		return _str[_pos++];
	}
	void stringbuf::unget() {
		if(!(_mode & ios_base::in) || _pos == 0)
			throw bad_state(string("No read-permission or unable to move back"));
		_pos--;
	}

	void stringbuf::put(char_type c) {
		if(!(_mode & ios_base::out))
			throw bad_state(string("No write-permission"));
		if(_mode & ios_base::app)
			_pos = _str.length();
		_str.insert(_pos++,&c,1);
	}

	void stringbuf::flush() {
	}
}
