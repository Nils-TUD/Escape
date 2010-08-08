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

namespace std {
	// === constructors ===
	string::string()
		: _str(new char[INIT_SIZE]), _size(INIT_SIZE), _length(0) {
		_str[0] = '\0';
	}
	string::string(const string& str)
		: _str(new char[str._size]), _size(str._size), _length(str._length) {
		memcpy(_str,str._str,_size * sizeof(char));
	}
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

	// === destructor ===
	string::~string() {
		delete[] _str;
	}

	// === operator=() ===
	string& string::operator=(const string& str) {
		return assign(str);
	}
	string& string::operator=(const char* s) {
		return assign(s);
	}
	string& string::operator=(char c) {
		delete[] _str;
		_length = 1;
		_size = 2;
		_str = new char[_size];
		_str[0] = c;
		_str[1] = '\0';
		return *this;
	}

	// === iterator stuff ===
	string::iterator string::begin() {
		return _str;
	}
	string::const_iterator string::begin() const {
		return _str;
	}
	string::iterator string::end() {
		return _str + _length;
	}
	string::const_iterator string::end() const {
		return _str + _length;
	}
	string::reverse_iterator string::rbegin() {
		return reverse_iterator(_str + _length);
	}
	string::const_reverse_iterator string::rbegin() const {
		return const_reverse_iterator(_str + _length);
	}
	string::reverse_iterator string::rend() {
		return reverse_iterator(_str);
	}
	string::const_reverse_iterator string::rend() const {
		return const_reverse_iterator(_str);
	}

	// === sizes ===
	string::size_type string::size() const {
		return _length;
	}
	string::size_type string::length() const {
		return _length;
	}
	string::size_type string::max_size() const {
		return _size - 1;
	}
	string::size_type string::capacity() const {
		return _size;
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
	void string::resize(size_type n) {
		resize(n,'\0');
	}
	void string::reserve(size_type res_arg) {
		if(_size < res_arg + 1)
			resize(res_arg);
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
	bool string::empty() const {
		return _length == 0;
	}

	// === operator[]() ===
	string::const_reference string::operator[](size_type pos) const {
		if(pos >= _length)
			throw out_of_range("Index out of range");
		return _str[pos];
	}
	string::reference string::operator[](size_type pos) {
		if(pos >= _length)
			throw out_of_range("Index out of range");
		return _str[pos];
	}

	// === at() ===
	string::const_reference string::at(size_type pos) const {
		return _str[pos];
	}
	string::reference string::at(size_type pos) {
		return _str[pos];
	}

	// === operator+=() ===
	string& string::operator+=(const string& str) {
		return append(str);
	}
	string& string::operator+=(const char* s) {
		return append(s);
	}
	string& string::operator+=(char c) {
		return append(&c,1);
	}

	// === append() ===
	string& string::append(const string& str) {
		return insert(_length,str);
	}
	string& string::append(const string& str,size_type pos,
			size_type n) {
		return insert(_length,str,pos,n);
	}
	string& string::append(const char* s,size_type n) {
		return insert(_length,s,n);
	}
	string& string::append(const char* s) {
		return append(s,strlen(s));
	}
	string& string::append(size_type n,char c) {
		return insert(_length,n,c);
	}
	void string::push_back(char c) {
		append(&c,1);
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
	string& string::assign(const char* s) {
		return assign(s,strlen(s));
	}
	string& string::assign(size_type n,char c) {
		clear();
		append(n,c);
		return *this;
	}

	// === insert() ===
	string& string::insert(size_type pos1,const string& str) {
		return insert(pos1,str,0,str._length);
	}
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
	string& string::insert(size_type pos1,const char* s,size_type n) {
		return insert(pos1,string(s),0,n);
	}
	string& string::insert(size_type pos1,const char* s) {
		string tmp(s);
		return insert(pos1,tmp,0,tmp._length);
	}
	string& string::insert(size_type pos1,size_type n,char c) {
		string tmp(n,c);
		return insert(pos1,tmp);
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
	string::iterator string::erase(iterator position) {
		return erase(position,position + 1);
	}
	string::iterator string::erase(iterator first,iterator last) {
		size_type n = distance(begin(),first);
		erase(size_type(first - begin()),distance(first,last));
		return begin() + n;
	}

	// === replace() ===
	string& string::replace(size_type pos1,size_type n1,
			const string& str) {
		return replace(pos1,n1,str,0,str._length);
	}
	string& string::replace(iterator i1,iterator i2,
			const string& str) {
		return replace(distance(begin(),i1),distance(i1,i2),str,0,str._length);
	}
	string& string::replace(size_type pos1,size_type n1,
			const string& str,size_type pos2,size_type n2) {
		erase(pos1,n1);
		return insert(pos1,str,pos2,n2);
	}
	string& string::replace(size_type pos1,size_type n1,const char* s,
			size_type n2) {
		string tmp(s);
		return replace(pos1,n1,tmp,0,n2);
	}
	string& string::replace(iterator i1,iterator i2,const char* s,
			size_type n2) {
		string tmp(s);
		return replace(distance(begin(),i1),distance(i1,i2),tmp,0,n2);
	}
	string& string::replace(size_type pos1,size_type n1,const char* s) {
		return replace(pos1,n1,s,strlen(s));
	}
	string& string::replace(iterator i1,iterator i2,const char* s) {
		return replace(distance(begin(),i1),distance(i1,i2),s,strlen(s));
	}
	string& string::replace(size_type pos1,size_type n1,size_type n2,char c) {
		string tmp(n2,c);
		return replace(pos1,n1,tmp);
	}
	string& string::replace(iterator i1,iterator i2,size_type n2,char c) {
		string tmp(n2,c);
		return replace(distance(begin(),i1),distance(i1,i2),tmp);
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

	// === data ===
	string::const_pointer string::c_str() const {
		return _str;
	}
	string::const_pointer string::data() const {
		return _str;
	}

	// === find() ===
	string::size_type string::find(const string& str,size_type pos) const {
		return find(str._str,pos,str._length);
	}
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
	string::size_type string::find(const char* s,size_type pos) const {
		return find(s,pos,strlen(s));
	}
	string::size_type string::find(char c,size_type pos) const {
		return find(&c,pos,1);
	}

	// === rfind() ===
	string::size_type string::rfind(const string& str,size_type pos) const {
		return rfind(str._str,pos,str._length);
	}
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
	string::size_type string::rfind(const char* s,size_type pos) const {
		return rfind(s,pos,strlen(s));
	}
	string::size_type string::rfind(char c,size_type pos) const {
		return rfind(&c,pos,1);
	}

	// === find_first_of() ===
	string::size_type string::find_first_of(const string& str,size_type pos) const {
		return find_first_of(str._str,pos,str._length);
	}
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
	string::size_type string::find_first_of(const char* s,size_type pos) const {
		return find_first_of(s,pos,strlen(s));
	}
	string::size_type string::find_first_of(char c,size_type pos) const {
		return find_first_of(&c,pos,1);
	}

	// === find_last_of() ===
	string::size_type string::find_last_of(const string& str,size_type pos) const {
		return find_last_of(str._str,pos,str._length);
	}
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
	string::size_type string::find_last_of(const char* s,size_type pos) const {
		return find_last_of(s,pos,strlen(s));
	}
	string::size_type string::find_last_of(char c,size_type pos) const {
		return find_last_of(&c,pos,1);
	}

	// === find_first_not_of() ===
	string::size_type string::find_first_not_of(const string& str,size_type pos) const {
		return find_first_not_of(str._str,pos,str._length);
	}
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
	string::size_type string::find_first_not_of(const char* s,size_type pos) const {
		return find_first_not_of(s,pos,strlen(s));
	}
	string::size_type string::find_first_not_of(char c,size_type pos) const {
		return find_first_not_of(&c,pos,1);
	}

	// === find_last_not_of() ===
	string::size_type string::find_last_not_of(const string& str,size_type pos) const {
		return find_last_not_of(str._str,pos,str._length);
	}
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
	string::size_type string::find_last_not_of(const char* s,size_type pos) const {
		return find_last_not_of(s,pos,strlen(s));
	}
	string::size_type string::find_last_not_of(char c,size_type pos) const {
		return find_last_not_of(&c,pos,1);
	}

	// === substr() ===
	string string::substr(size_type pos,size_type n) const {
		return string(*this,pos,n);
	}

	// === compare() ===
	int string::compare(const string& str) const {
		return strcmp(_str,str._str);
	}
	int string::compare(const char* s) const {
		return strcmp(_str,s);
	}
	int string::compare(size_type pos1,size_type n1,const string& str) const {
		return strncmp(_str + pos1,str._str,n1);
	}
	int string::compare(size_type pos1,size_type n1,const char* s) const {
		return strncmp(_str + pos1,s,n1);
	}
	int string::compare(size_type pos1,size_type n1,const string& str,
			size_type pos2,size_type n2) const {
		return substr(pos1,n1).compare(str.substr(pos2,n2));
	}
	int string::compare(size_type pos1,size_type n1,const char* s,size_type n2) const {
		return strncmp(substr(pos1,n1)._str,s,n2);
	}

	// === global operator+ ===
	string operator+(const string& lhs,const string& rhs) {
		return string(lhs).append(rhs);
	}
	string operator+(const char* lhs,const string& rhs) {
		return string(lhs).append(rhs);
	}
	string operator+(char lhs,const string& rhs) {
		return string(1,lhs).append(rhs);
	}
	string operator+(const string& lhs,const char* rhs) {
		return string(lhs).append(rhs);
	}
	string operator+(const string& lhs,char rhs) {
		return string(lhs).append(&rhs,1);
	}

	// === global swap ===
	void swap(string& lhs,string& rhs) {
		lhs.swap(rhs);
	}

	// === global comparison-operators ===
	bool operator==(const string& lhs,const string& rhs) {
		return lhs.compare(rhs) == 0;
	}
	bool operator==(const char* lhs,const string& rhs) {
		return rhs.compare(lhs) == 0;
	}
	bool operator==(const string& lhs,const char* rhs) {
		return lhs.compare(rhs) == 0;
	}
	bool operator!=(const string& lhs,const string& rhs) {
		return lhs.compare(rhs) != 0;
	}
	bool operator!=(const char* lhs,const string& rhs) {
		return rhs.compare(lhs) != 0;
	}
	bool operator!=(const string& lhs,const char* rhs) {
		return lhs.compare(rhs) != 0;
	}
	bool operator<(const string& lhs,const string& rhs) {
		return lhs.compare(rhs) < 0;
	}
	bool operator<(const char* lhs,const string& rhs) {
		return rhs.compare(lhs) > 0;
	}
	bool operator<(const string& lhs,const char* rhs) {
		return lhs.compare(rhs) < 0;
	}
	bool operator>(const string& lhs,const string& rhs) {
		return lhs.compare(rhs) > 0;
	}
	bool operator>(const char* lhs,const string& rhs) {
		return rhs.compare(lhs) < 0;
	}
	bool operator>(const string& lhs,const char* rhs) {
		return lhs.compare(rhs) > 0;
	}
	bool operator<=(const string& lhs,const string& rhs) {
		return lhs.compare(rhs) <= 0;
	}
	bool operator<=(const char* lhs,const string& rhs) {
		return rhs.compare(lhs) >= 0;
	}
	bool operator<=(const string& lhs,const char* rhs) {
		return lhs.compare(rhs) <= 0;
	}
	bool operator>=(const string& lhs,const string& rhs) {
		return lhs.compare(rhs) >= 0;
	}
	bool operator>=(const char* lhs,const string& rhs) {
		return rhs.compare(lhs) <= 0;
	}
	bool operator>=(const string& lhs,const char* rhs) {
		return lhs.compare(rhs) >= 0;
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

	istream& getline(istream& is,string& str) {
		return getline(is,str,'\n');
	}
	istream& getline(istream& is,string& str,char delim) {
		stringbuf sb(str);
		is.getline(sb,delim);
		str = sb.str();
		return is;
	}
}
