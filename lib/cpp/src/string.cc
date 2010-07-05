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

#include <stdlib.h>
#include <string.h>
#include <string>

namespace std {
	// === constructors ===
	template<typename T>
	inline basic_string<T>::basic_string()
		: _str(new char[INIT_SIZE]), _size(INIT_SIZE), _length(0) {
		_str[0] = '\0';
	}
	template<typename T>
	basic_string<T>::basic_string(const basic_string<T>& str)
		: _str(new char[str._size]), _size(str._size), _length(str._length) {
		memcpy(_str,str._str,_size);
	}
	template<typename T>
	basic_string<T>::basic_string(const basic_string<T>& str,size_type pos,size_type n)
		: _str(NULL), _size(0), _length(0) {
		if(n == npos)
			n = str._length - pos;
		assign(str,pos,n);
	}
	template<typename T>
	basic_string<T>::basic_string(const T* s,size_type n)
		: _str(new char[n + 1]), _size(n + 1), _length(n) {
		memcpy(_str,s,n);
		_str[n] = '\0';
	}
	template<typename T>
	basic_string<T>::basic_string(const T* s)
		: _str(NULL), _size(0), _length(0) {
		size_type len = strlen(s);
		_str = new char[len + 1];
		_size = len + 1;
		_length = len;
		memcpy(_str,s,len);
		_str[len] = '\0';
	}
	template<typename T>
	basic_string<T>::basic_string(size_type n,T c)
		: _str(new char[n + 1]), _size(n + 1), _length(n) {
		_str[n] = '\0';
		for(; n > 0; n--)
			_str[n - 1] = c;
	}

	// === destructor ===
	template<typename T>
	inline basic_string<T>::~basic_string() {
		delete _str;
	}

	// === operator=() ===
	template<typename T>
	basic_string<T>& basic_string<T>::operator=(const basic_string<T>& str) {
		return assign(str);
	}
	template<typename T>
	basic_string<T>& basic_string<T>::operator=(const T* s) {
		return assign(s);
	}
	template<typename T>
	basic_string<T>& basic_string<T>::operator=(T c) {
		delete _str;
		_length = 1;
		_size = 2;
		_str = new char[_size];
		_str[0] = c;
		_str[1] = '\0';
		return *this;
	}

	// === sizes ===
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::size() const {
		return _length;
	}
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::length() const {
		return _length;
	}
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::max_size() const {
		return _size - 1;
	}
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::capacity() const {
		return _size;
	}

	// === resize() and reserve() ===
	template<typename T>
	void basic_string<T>::resize(size_type n,T c) {
		if(n + 1 < _size) {
			_size = n + 1;
			if(n < _length) {
				_length = n;
				_str[_length] = '\0';
			}
		}
		else if(n + 1 > _size) {
			T *tmp = new char[n + 1];
			if(_size > 0) {
				memcpy(tmp,_str,_size - 1);
				tmp[_size - 1] = c;
			}
			for(size_type i = _size; i < n; i++)
				tmp[i] = c;
			tmp[n] = '\0';
			delete _str;
			_str = tmp;
			_size = n + 1;
		}
	}
	template<typename T>
	inline void basic_string<T>::resize(size_type n) {
		resize(n,'\0');
	}
	template<typename T>
	inline void basic_string<T>::reserve(size_type res_arg) {
		if(_size < res_arg + 1)
			resize(res_arg);
	}

	// === clear() and empty() ===
	template<typename T>
	void basic_string<T>::clear() {
		if(_size > INIT_SIZE) {
			delete _str;
			_str = new char[INIT_SIZE];
			_size = INIT_SIZE;
		}
		_str[0] = '\0';
		_length = 0;
	}
	template<typename T>
	inline bool basic_string<T>::empty() const {
		return _length == 0;
	}

	// === operator[]() ===
	template<typename T>
	inline typename basic_string<T>::const_reference basic_string<T>::operator[](size_type pos) const {
		if(pos >= _length)
			exit(1); // TODO throw exception
		return _str[pos];
	}
	template<typename T>
	inline typename basic_string<T>::reference basic_string<T>::operator[](size_type pos) {
		if(pos >= _length)
			exit(1); // TODO throw exception
		return _str[pos];
	}

	// === at() ===
	template<typename T>
	inline typename basic_string<T>::const_reference basic_string<T>::at(size_type pos) const {
		return _str[pos];
	}
	template<typename T>
	inline typename basic_string<T>::reference basic_string<T>::at(size_type pos) {
		return _str[pos];
	}

	// === operator+=() ===
	template<typename T>
	inline basic_string<T>& basic_string<T>::operator+=(const basic_string& str) {
		return append(str);
	}
	template<typename T>
	inline basic_string<T>& basic_string<T>::operator+=(const T* s) {
		return append(s);
	}
	template<typename T>
	inline basic_string<T>& basic_string<T>::operator+=(char c) {
		return append(c);
	}

	// === append() ===
	template<typename T>
	inline basic_string<T>& basic_string<T>::append(const basic_string<T>& str) {
		return insert(_length,str);
	}
	template<typename T>
	inline basic_string<T>& basic_string<T>::append(const basic_string<T>& str,size_type pos,
			size_type n) {
		return insert(_length,str,pos,n);
	}
	template<typename T>
	inline basic_string<T>& basic_string<T>::append(const T* s,size_type n) {
		return insert(_length,s,n);
	}
	template<typename T>
	inline basic_string<T>& basic_string<T>::append(const T* s) {
		return append(s,strlen(s));
	}
	template<typename T>
	inline basic_string<T>& basic_string<T>::append(size_type n,T c) {
		return insert(_length,n,c);
	}
	template<typename T>
	inline void basic_string<T>::push_back(T c) {
		append(c);
	}

	// === assign() ===
	template<typename T>
	basic_string<T>& basic_string<T>::assign(const basic_string<T>& str) {
		delete _str;
		_length = str._length;
		_size = str._size;
		_str = new char[_size];
		memcpy(_str,str._str,_size);
		return *this;
	}
	template<typename T>
	basic_string<T>& basic_string<T>::assign(const basic_string<T>& str,size_type pos,size_type n) {
		delete _str;
		if(pos > str._length || (n != npos && pos + n < pos))
			exit(1); // TODO throw exception
		if(n == npos || pos + n > str._length)
			n = str._length - pos;
		_size = n + 1;
		_str = new char[_size];
		memcpy(_str,str._str + pos,n);
		_length = n;
		_str[_length] = '\0';
		return *this;
	}
	template<typename T>
	basic_string<T>& basic_string<T>::assign(const T* s,size_type n) {
		delete _str;
		_size = n + 1;
		_str = new char[_size];
		memcpy(_str,s,n);
		_length = n;
		_str[_length] = '\0';
		return *this;
	}
	template<typename T>
	basic_string<T>& basic_string<T>::assign(const T* s) {
		return assign(s,strlen(s));
	}
	template<typename T>
	basic_string<T>& basic_string<T>::assign(size_type n,T c) {
		delete _str;
		_size = n + 1;
		_str = new char[_size];
		for(size_type i = 0; i < n; i++)
			_str[i] = c;
		_length = n;
		return *this;
	}

	// === insert() ===
	template<typename T>
	inline basic_string<T>& basic_string<T>::insert(size_type pos1,const basic_string<T>& str) {
		return insert(pos1,str,0,str._length);
	}
	template<typename T>
	basic_string<T>& basic_string<T>::insert(size_type pos1,const basic_string<T>& str,
			size_type pos2,size_type n) {
		if(pos1 > _length)
			exit(1); // TODO throw exception
		if(pos2 > str._length)
			exit(1); // TODO throw exception
		if(pos2 + n > str._length)
			n = pos2 - str._length;
		reserve(_length + n);
		if(pos1 < _length)
			memmove(_str + pos1 + n,_str + pos1,_length - pos1);
		memcpy(_str + pos1,str._str + pos2,n);
		_length += n;
		_str[_length] = '\0';
		return *this;
	}
	template<typename T>
	inline basic_string<T>& basic_string<T>::insert(size_type pos1,const T* s,size_type n) {
		return insert(pos1,string(s),0,n);
	}
	template<typename T>
	inline basic_string<T>& basic_string<T>::insert(size_type pos1,const T* s) {
		string tmp(s);
		return insert(pos1,tmp,0,tmp._length);
	}
	template<typename T>
	inline basic_string<T>& basic_string<T>::insert(size_type pos1,size_type n,T c) {
		string tmp(n,c);
		return insert(pos1,tmp);
	}

	// === erase() ===
	template<typename T>
	basic_string<T>& basic_string<T>::erase(size_type pos,size_type n) {
		if(pos > _length)
			exit(1); // TODO throw exception
		if(n == npos)
			n = _length - pos;
		if(pos + n > _length)
			n = _length - pos;
		if(pos + n < _length)
			memmove(_str + pos,_str + pos + n,_length - (pos + n));
		_length -= n;
		_str[_length] = '\0';
		return *this;
	}

	// === replace() ===
	template<typename T>
	inline basic_string<T>& basic_string<T>::replace(size_type pos1,size_type n1,
			const basic_string& str) {
		return replace(pos1,n1,str,0,str._length);
	}
	template<typename T>
	inline basic_string<T>& basic_string<T>::replace(size_type pos1,size_type n1,
			const basic_string& str,size_type pos2,size_type n2) {
		erase(pos1,n1);
		return insert(pos1,str,pos2,n2);
	}
	template<typename T>
	inline basic_string<T>& basic_string<T>::replace(size_type pos1,size_type n1,const T* s,
			size_type n2) {
		string tmp(s);
		return replace(pos1,n1,tmp,0,n2);
	}
	template<typename T>
	inline basic_string<T>& basic_string<T>::replace(size_type pos1,size_type n1,const T* s) {
		return replace(pos1,n1,s,strlen(s));
	}
	template<typename T>
	inline basic_string<T>& basic_string<T>::replace(size_type pos1,size_type n1,size_type n2,T c) {
		string tmp(n2,c);
		return replace(pos1,n1,tmp);
	}

	// === copy() and swap ===
	template<typename T>
	typename basic_string<T>::size_type basic_string<T>::copy(T* s,size_type n,size_type pos) const {
		if(pos > _length)
			exit(1); // TODO throw exception
		if(pos + n > _length)
			n = _length - pos;
		memcpy(s,_str + pos,n);
		return n;
	}
	template<typename T>
	void basic_string<T>::swap(basic_string<T>& str) {
		string tmp(str);
		str.assign(this);
		this.assign(tmp);
	}

	// === data ===
	template<typename T>
	inline typename basic_string<T>::const_pointer basic_string<T>::c_str() const {
		return _str;
	}
	template<typename T>
	inline typename basic_string<T>::const_pointer basic_string<T>::data() const {
		return _str;
	}

	// === find() ===
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::find(const basic_string& str,
			size_type pos) const {
		return find(str._str,pos,str._length);
	}
	template<typename T>
	typename basic_string<T>::size_type basic_string<T>::find(const T* s,size_type pos,
			size_type n) const {
		// handle special case to prevent looping the string
		if(n == 0)
			return npos;
		T *str1 = _str + pos;
		for(size_type i = pos; *str1; i++) {
			if(*str1++ == *s && strncmp(str1 - 1,s,n) == 0)
				return i;
		}
		return npos;
	}
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::find(const T* s,size_type pos) const {
		return find(s,pos,strlen(s));
	}
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::find(T c,size_type pos) const {
		return find(&c,pos,1);
	}

	// === rfind() ===
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::rfind(const basic_string<T>& str,
			size_type pos) const {
		return rfind(str._str,pos,str._length);
	}
	template<typename T>
	typename basic_string<T>::size_type basic_string<T>::rfind(const T* s,size_type pos,
			size_type n) const {
		// handle special case to prevent looping the string
		if(n == 0 || pos < (n - 1))
			return npos;
		if(pos == npos)
			pos = _length - 1;
		T *str1 = _str + pos - (n - 1);
		for(size_type i = pos - (n - 1); ; i--) {
			if(*str1-- == *s && strncmp(str1 + 1,s,n) == 0)
				return i;
			if(i == 0)
				break;
		}
		return npos;
	}
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::rfind(const T* s,size_type pos) const {
		return rfind(s,pos,strlen(s));
	}
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::rfind(T c,size_type pos) const {
		return rfind(&c,pos,1);
	}

	// === find_first_of() ===
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::find_first_of(const basic_string& str,
			size_type pos) const {
		return find_first_of(str._str,pos,str._length);
	}
	template<typename T>
	typename basic_string<T>::size_type basic_string<T>::find_first_of(const T* s,size_type pos,
			size_type n) const {
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
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::find_first_of(const T* s,
			size_type pos) const {
		return find_first_of(s,pos,strlen(s));
	}
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::find_first_of(T c,
			size_type pos) const {
		return find_first_of(&c,pos,1);
	}

	// === find_last_of() ===
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::find_last_of(
			const basic_string<T>& str,size_type pos) const {
		return find_last_of(str._str,pos,str._length);
	}
	template<typename T>
	typename basic_string<T>::size_type basic_string<T>::find_last_of(const T* s,size_type pos,
			size_type n) const {
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
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::find_last_of(const T* s,
			size_type pos) const {
		return find_last_of(s,pos,strlen(s));
	}
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::find_last_of(T c,
			size_type pos) const {
		return find_last_of(&c,pos,1);
	}

	// === find_first_not_of() ===
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::find_first_not_of(
			const basic_string<T>& str,size_type pos) const {
		return find_first_not_of(str._str,pos,str._length);
	}
	template<typename T>
	typename basic_string<T>::size_type basic_string<T>::find_first_not_of(const T* s,
			size_type pos,size_type n) const {
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
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::find_first_not_of(const T* s,
			size_type pos) const {
		return find_first_not_of(s,pos,strlen(s));
	}
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::find_first_not_of(T c,
			size_type pos) const {
		return find_first_not_of(&c,pos,1);
	}

	// === find_last_not_of() ===
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::find_last_not_of(
			const basic_string<T>& str,size_type pos) const {
		return find_last_not_of(str._str,pos,str._length);
	}
	template<typename T>
	typename basic_string<T>::size_type basic_string<T>::find_last_not_of(const T* s,
			size_type pos,size_type n) const {
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
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::find_last_not_of(const T* s,
			size_type pos) const {
		return find_last_not_of(s,pos,strlen(s));
	}
	template<typename T>
	inline typename basic_string<T>::size_type basic_string<T>::find_last_not_of(T c,
			size_type pos) const {
		return find_last_not_of(&c,pos,1);
	}

	// === substr() ===
	template<typename T>
	basic_string<T> basic_string<T>::substr(size_type pos,size_type n) const {
		return basic_string<T>(this,pos,n);
	}

	// === compare() ===
	template<typename T>
	int basic_string<T>::compare(const basic_string<T>& str) const {
		return strcmp(_str,str._str);
	}
	template<typename T>
	int basic_string<T>::compare(const T* s) const {
		return strcmp(_str,s);
	}
	template<typename T>
	int basic_string<T>::compare(size_type pos1,size_type n1,const basic_string<T>& str) const {
		return strncmp(_str + pos1,str._str,n1);
	}
	template<typename T>
	int basic_string<T>::compare(size_type pos1,size_type n1,const T* s) const {
		return strncmp(_str + pos1,s,n1);
	}
	template<typename T>
	int basic_string<T>::compare(size_type pos1,size_type n1,const basic_string<T>& str,
			size_type pos2,size_type n2) const {
		return substr(pos1,n1).compare(str.substr(pos2,n2));
	}
	template<typename T>
	int basic_string<T>::compare(size_type pos1,size_type n1,const T* s,size_type n2) const {
		return strncmp(substr(pos1,n1)._str,s,n2);
	}


	// === global operator+ ===
	template<typename T>
	basic_string<T> operator+(const basic_string<T>& lhs,const basic_string<T>& rhs) {
		return basic_string<T>(lhs).append(rhs);
	}
	template<typename T>
	basic_string<T> operator+(const T* lhs,const basic_string<T>& rhs) {
		return basic_string<T>(lhs).append(rhs);
	}
	template<typename T>
	basic_string<T> operator+(T lhs,const basic_string<T>& rhs) {
		return basic_string<T>(1,lhs).append(rhs);
	}
	template<typename T>
	basic_string<T> operator+(const basic_string<T>& lhs,const T* rhs) {
		return basic_string<T>(lhs).append(rhs);
	}
	template<typename T>
	basic_string<T> operator+(const basic_string<T>& lhs,T rhs) {
		return basic_string<T>(lhs).append(rhs);
	}

	// === global swap ===
	template<typename T>
	void swap(basic_string<T>& lhs,basic_string<T>& rhs) {
		lhs.swap(rhs);
	}

	// === global comparison-operators ===
	template<typename T>
	bool operator==(const basic_string<T>& lhs,const basic_string<T>& rhs) {
		return lhs.compare(rhs) == 0;
	}
	template<typename T>
	bool operator==(const T* lhs,const basic_string<T>& rhs) {
		return rhs.compare(lhs) == 0;
	}
	template<typename T>
	bool operator==(const basic_string<T>& lhs,const T* rhs) {
		return lhs.compare(rhs) == 0;
	}
	template<typename T>
	bool operator!=(const basic_string<T>& lhs,const basic_string<T>& rhs) {
		return lhs.compare(rhs) != 0;
	}
	template<typename T>
	bool operator!=(const T* lhs,const basic_string<T>& rhs) {
		return rhs.compare(lhs) != 0;
	}
	template<typename T>
	bool operator!=(const basic_string<T>& lhs,const T* rhs) {
		return lhs.compare(rhs) != 0;
	}
	template<typename T>
	bool operator<(const basic_string<T>& lhs,const basic_string<T>& rhs) {
		return lhs.compare(rhs) < 0;
	}
	template<typename T>
	bool operator<(const T* lhs,const basic_string<T>& rhs) {
		return rhs.compare(lhs) > 0;
	}
	template<typename T>
	bool operator<(const basic_string<T>& lhs,const T* rhs) {
		return lhs.compare(rhs) < 0;
	}
	template<typename T>
	bool operator>(const basic_string<T>& lhs,const basic_string<T>& rhs) {
		return lhs.compare(rhs) > 0;
	}
	template<typename T>
	bool operator>(const T* lhs,const basic_string<T>& rhs) {
		return rhs.compare(lhs) < 0;
	}
	template<typename T>
	bool operator>(const basic_string<T>& lhs,const T* rhs) {
		return lhs.compare(rhs) > 0;
	}
	template<typename T>
	bool operator<=(const basic_string<T>& lhs,const basic_string<T>& rhs) {
		return lhs.compare(rhs) <= 0;
	}
	template<typename T>
	bool operator<=(const T* lhs,const basic_string<T>& rhs) {
		return rhs.compare(lhs) >= 0;
	}
	template<typename T>
	bool operator<=(const basic_string<T>& lhs,const T* rhs) {
		return lhs.compare(rhs) <= 0;
	}
	template<typename T>
	bool operator>=(const basic_string<T>& lhs,const basic_string<T>& rhs) {
		return lhs.compare(rhs) >= 0;
	}
	template<typename T>
	bool operator>=(const T* lhs,const basic_string<T>& rhs) {
		return rhs.compare(lhs) <= 0;
	}
	template<typename T>
	bool operator>=(const basic_string<T>& lhs,const T* rhs) {
		return lhs.compare(rhs) >= 0;
	}
}
