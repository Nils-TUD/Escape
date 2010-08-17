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

#include <string>
#include <exception>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <ctype.h>

namespace std {
	// === constructors ===
	string::string(const string& str,size_type pos,size_type n)
		: _str(NULL), _size(0), _length(0) {
		if(n == npos)
			n = str._length - pos;
		assign(str,pos,n);
	}
	string::string(const char* s,size_type n)
		: _str(new char[n + 1]), _size(n + 1), _length(n) {
		memcpy(_str,s,n);
		_str[n] = '\0';
	}
	string::string(const char* s)
		: _str(NULL), _size(0), _length(0) {
		size_type len = strlen(s);
		_str = new char[len + 1];
		_size = len + 1;
		_length = len;
		memcpy(_str,s,len * sizeof(char));
		_str[len] = '\0';
	}
	string::string(size_type n,char c)
		: _str(new char[n + 1]), _size(n + 1), _length(n) {
		_str[n] = '\0';
		for(; n > 0; n--)
			_str[n - 1] = c;
	}

	// === operator=() ===
	string& string::operator=(char c) {
		delete[] _str;
		_length = 1;
		_size = 2;
		_str = new char[_size];
		_str[0] = c;
		_str[1] = '\0';
		return *this;
	}

	// === resize() and reserve() ===
	void string::resize(size_type n,char c) {
		if(n + 1 < _size) {
			_size = n + 1;
			if(n < _length) {
				_length = n;
				_str[_length] = '\0';
			}
		}
		else if(n + 1 > _size) {
			char *tmp = new char[n + 1];
			if(_size > 0) {
				memcpy(tmp,_str,(_size - 1) * sizeof(char));
				tmp[_size - 1] = c;
			}
			for(size_type i = _size; i < n; i++)
				tmp[i] = c;
			tmp[n] = '\0';
			delete[] _str;
			_str = tmp;
			_size = n + 1;
		}
	}

	// === clear() and empty() ===
	void string::clear() {
		if(_size > INIT_SIZE || !_str) {
			delete[] _str;
			_str = new char[INIT_SIZE];
			_size = INIT_SIZE;
		}
		_str[0] = '\0';
		_length = 0;
	}

	// === at() ===
	string::const_reference string::at(size_type pos) const {
		if(pos >= _length)
			throw out_of_range("Index out of range");
		return _str[pos];
	}
	string::reference string::at(size_type pos) {
		if(pos >= _length)
			throw out_of_range("Index out of range");
		return _str[pos];
	}

	// === assign() ===
	string& string::assign(const string& str) {
		clear();
		append(str);
		return *this;
	}
	string& string::assign(const string& str,size_type pos,size_type n) {
		if(pos > str._length || (n != npos && pos + n < pos))
			throw out_of_range("Index out of range");
		clear();
		append(str,pos,n);
		return *this;
	}
	string& string::assign(const char* s,size_type n) {
		clear();
		append(s,n);
		return *this;
	}
	string& string::assign(size_type n,char c) {
		clear();
		append(n,c);
		return *this;
	}

	// === insert() ===
	string& string::insert(size_type pos1,const string& str,
			size_type pos2,size_type n) {
		if(pos1 > _length)
			throw out_of_range("pos1 out of range");
		if(pos2 > str._length)
			throw out_of_range("pos2 out of range");
		if(pos2 + n > str._length)
			n = pos2 - str._length;
		reserve(_length + n);
		if(pos1 < _length)
			memmove(_str + pos1 + n,_str + pos1,(_length - pos1) * sizeof(char));
		memcpy(_str + pos1,str._str + pos2,n * sizeof(char));
		_length += n;
		_str[_length] = '\0';
		return *this;
	}

	// === erase() ===
	string& string::erase(size_type pos,size_type n) {
		if(pos > _length)
			throw out_of_range("Index out of range");
		if(n == npos)
			n = _length - pos;
		if(pos + n > _length)
			n = _length - pos;
		if(pos + n < _length)
			memmove(_str + pos,_str + pos + n,(_length - (pos + n)) * sizeof(char));
		_length -= n;
		_str[_length] = '\0';
		return *this;
	}
	string::iterator string::erase(iterator first,iterator last) {
		size_type n = distance(begin(),first);
		erase(size_type(first - begin()),distance(first,last));
		return begin() + n;
	}

	// === copy() and swap ===
	string::size_type string::copy(char* s,size_type n,size_type pos) const {
		if(pos > _length)
			throw out_of_range("Index out of range");
		if(pos + n > _length)
			n = _length - pos;
		memcpy(s,_str + pos,n * sizeof(char));
		return n;
	}
	void string::swap(string& str) {
		string tmp(str);
		str.assign(*this);
		this->assign(tmp);
	}

	// === find() ===
	string::size_type string::find(const char* s,size_type pos,size_type n) const {
		// handle special case to prevent looping the string
		if(n == 0)
			return npos;
		char *str1 = _str + pos;
		for(size_type i = pos; *str1; i++) {
			if(*str1++ == *s && strncmp(str1 - 1,s,n) == 0)
				return i;
		}
		return npos;
	}

	// === rfind() ===
	string::size_type string::rfind(const char* s,size_type pos,size_type n) const {
		// handle special case to prevent looping the string
		if(n == 0 || pos < (n - 1))
			return npos;
		if(pos == npos)
			pos = _length - 1;
		char *str1 = _str + pos - (n - 1);
		for(size_type i = pos - (n - 1); ; i--) {
			if(*str1-- == *s && strncmp(str1 + 1,s,n) == 0)
				return i;
			if(i == 0)
				break;
		}
		return npos;
	}

	// === find_first_of() ===
	string::size_type string::find_first_of(const char* s,size_type pos,size_type n) const {
		if(n == 0)
			return npos;
		for(size_type i = pos; i < _length; i++) {
			for(size_type j = 0; j < n; j++) {
				if(_str[i] == s[j])
					return i;
			}
		}
		return npos;
	}

	// === find_last_of() ===
	string::size_type string::find_last_of(const char* s,size_type pos,size_type n) const {
		if(n == 0)
			return npos;
		if(pos == npos)
			pos = _length - 1;
		for(size_type i = pos; ; i--) {
			for(size_type j = 0; j < n; j++) {
				if(_str[i] == s[j])
					return i;
			}
			if(i == 0)
				break;
		}
		return npos;
	}

	// === find_first_not_of() ===
	string::size_type string::find_first_not_of(const char* s,size_type pos,size_type n) const {
		for(size_type i = pos; i < _length; i++) {
			bool found = false;
			for(size_type j = 0; j < n; j++) {
				if(_str[i] == s[j]) {
					found = true;
					break;
				}
			}
			if(!found)
				return i;
		}
		return npos;
	}

	// === find_last_not_of() ===
	string::size_type string::find_last_not_of(const char* s,size_type pos,size_type n) const {
		if(pos == npos)
			pos = _length - 1;
		for(size_type i = pos; ; i--) {
			bool found = false;
			for(size_type j = 0; j < n; j++) {
				if(_str[i] == s[j]) {
					found = true;
					break;
				}
			}
			if(!found)
				return i;
			if(i == 0)
				break;
		}
		return npos;
	}


	// === trim ===
	string::size_type string::ltrim() {
		iterator it = begin();
		for(; it != end(); ++it) {
			if(!::isspace(*it))
				break;
		}
		size_type count = distance(begin(),it);
		erase(begin(),it);
		return count;
	}
	string::size_type string::rtrim() {
		reverse_iterator it = rbegin();
		for(; it != rend(); ++it) {
			if(!::isspace(*it))
				break;
		}
		size_type count = distance(rbegin(),it);
		erase(end() - count,end());
		return count;
	}

	// === stream stuff ===
	ostream& operator <<(ostream& os,const string& s) {
		stringbuf sb(s);
		os << &sb;
		return os;
	}
	istream& operator >>(istream& in,string& s) {
		ws(in);
		stringbuf sb(s);
		in.getword(sb);
		s = sb.str();
		return in;
	}

	istream& getline(istream& is,string& str,char delim) {
		stringbuf sb("");
		is.getline(sb,delim);
		str = sb.str();
		return is;
	}
}
