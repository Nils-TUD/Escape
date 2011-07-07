/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
	// === constructors ===
	template<class T>
	inline vector<T>::vector()
		: _count(0), _size(INITIAL_SIZE), _elements(new T[INITIAL_SIZE]) {
	}
	template<class T>
	vector<T>::vector(size_type n,const T& value)
		: _count(n), _size(n), _elements(new T[n]) {
		for(size_type i = 0; i < n; i++)
			_elements[i] = value;
	}
	template<class T>
	template<class InputIterator>
	vector<T>::vector(InputIterator first,InputIterator last)
		: _count(last - first), _size(last - first), _elements(new T[last - first]) {
		for(size_type i = 0; first < last; i++, first++)
			_elements[i] = *first;
	}
	template<class T>
	vector<T>::vector(const vector<T>& x)
		: _count(x._count), _size(x._count), _elements(new T[x._count]) {
		for(size_type i = 0; i < _count; i++)
			_elements[i] = x._elements[i];
	}

	// === destructors ===
	template<class T>
	inline vector<T>::~vector() {
		delete[] _elements;
	}

	// === assignment ===
	template<class T>
	vector<T>& vector<T>::operator =(const vector<T>& x) {
		delete[] _elements;
		_elements = new T[x._count];
		_count = x._count;
		_size = x._count;
		for(size_type i = 0; i < _count; i++)
			_elements[i] = x._elements[i];
		return *this;
	}
	template<class T>
	template<class InputIterator>
	void vector<T>::assign(InputIterator first,InputIterator last) {
		delete[] _elements;
		size_type count = last - first;
		_elements = new T[count];
		_count = count;
		_size = count;
		for(size_type i = 0; first < last; i++, first++)
			_elements[i] = *first;
	}
	template<class T>
	void vector<T>::assign(size_type n,const T& u) {
		delete[] _elements;
		_elements = new T[n];
		_count = n;
		_size = n;
		for(size_type i = 0; i < n; i++)
			_elements[i] = u;
	}

	// === iterator-stuff ===
	template<class T>
	inline typename vector<T>::iterator vector<T>::begin() {
		return iterator(_elements);
	}
	template<class T>
	inline typename vector<T>::const_iterator vector<T>::begin() const {
		return const_iterator(_elements);
	}
	template<class T>
	inline typename vector<T>::iterator vector<T>::end() {
		return iterator(_elements + _count);
	}
	template<class T>
	inline typename vector<T>::const_iterator vector<T>::end() const {
		return const_iterator(_elements + _count);
	}
	template<class T>
	inline typename vector<T>::reverse_iterator vector<T>::rbegin() {
		return reverse_iterator(_elements + _count);
	}
	template<class T>
	inline typename vector<T>::const_reverse_iterator vector<T>::rbegin() const {
		return const_reverse_iterator(_elements + _count);
	}
	template<class T>
	inline typename vector<T>::reverse_iterator vector<T>::rend() {
		return reverse_iterator(_elements);
	}
	template<class T>
	inline typename vector<T>::const_reverse_iterator vector<T>::rend() const {
		return const_reverse_iterator(_elements);
	}

	// === size-stuff ===
	template<class T>
	inline typename vector<T>::size_type vector<T>::size() const {
		return _count;
	}
	template<class T>
	inline typename vector<T>::size_type vector<T>::max_size() const {
		return ULONG_MAX / sizeof(T);
	}
	template<class T>
	void vector<T>::resize(size_type sz,T c) {
		if(sz < _count)
			_count = sz;
		else if(sz > _count)
			insert(_count,sz - _count,c);
	}
	template<class T>
	inline typename vector<T>::size_type vector<T>::capacity() const {
		return _size;
	}
	template<class T>
	inline bool vector<T>::empty() const {
		return _count == 0;
	}
	template<class T>
	inline void vector<T>::reserve(size_type n) {
		if(n > _size) {
			n = max(_size * 2,n);
			T *tmp = new T[n];
			for(size_type i = 0; i < _size; ++i)
				tmp[i] = _elements[i];
			delete[] _elements;
			_elements = tmp;
			_size = n;
		}
	}

	// === data-access ===
	template<class T>
	inline typename vector<T>::reference vector<T>::operator[](size_type n) {
		return _elements[n];
	}
	template<class T>
	inline typename vector<T>::const_reference vector<T>::operator[](size_type n) const {
		return _elements[n];
	}
	template<class T>
	typename vector<T>::reference vector<T>::at(size_type n) {
		if(n >= _count)
			throw out_of_range("Index out of range");
		return _elements[n];
	}
	template<class T>
	typename vector<T>::const_reference vector<T>::at(size_type n) const {
		if(n >= _count)
			throw out_of_range("Index out of range");
		return _elements[n];
	}
	template<class T>
	inline typename vector<T>::reference vector<T>::front() {
		return _elements[0];
	}
	template<class T>
	inline typename vector<T>::const_reference vector<T>::front() const {
		return _elements[0];
	}
	template<class T>
	inline typename vector<T>::reference vector<T>::back() {
		return _elements[_count - 1];
	}
	template<class T>
	inline typename vector<T>::const_reference vector<T>::back() const {
		return _elements[_count - 1];
	}
	template<class T>
	inline typename vector<T>::pointer vector<T>::data() {
		return _elements;
	}
	template<class T>
	inline typename vector<T>::const_pointer vector<T>::data() const {
		return _elements;
	}

	// === data-manipulation ===
	template<class T>
	inline void vector<T>::push_back(const T& x) {
		reserve(_count + 1);
		_elements[_count++] = x;
	}
	template<class T>
	inline void vector<T>::pop_back() {
		_count--;
	}
	template<class T>
	typename vector<T>::iterator vector<T>::insert(iterator position,const T& x) {
		size_type i = position - _elements;
		reserve(_count + 1);
		position = _elements + i;
		if(position < end())
			memmove(position + 1,position,(end() - position) * sizeof(T));
		*position = x;
		_count++;
		return position;
	}
	template<class T>
	void vector<T>::insert(iterator position,size_type n,const T& x) {
		size_type i = position - _elements;
		reserve(_count + n);
		position = _elements + i;
		if(position < end())
			memmove(position + n,position,(end() - position) * sizeof(T));
		for(size_type i = 0; i < n; i++)
			*position++ = x;
		_count += n;
	}
	template<class T>
	template<class InputIterator>
	void vector<T>::insert(iterator position,InputIterator first,InputIterator last) {
		size_type i = position - _elements;
		size_type n = last - first;
		reserve(_count + n);
		position = _elements + i;
		if(position < end())
			memmove(position + n,position,(end() - position) * sizeof(T));
		while(first < last)
			*position++ = *first++;
		_count += n;
	}
	template<class T>
	inline typename vector<T>::iterator vector<T>::erase(iterator position) {
		return erase(position,position + 1);
	}
	template<class T>
	typename vector<T>::iterator vector<T>::erase(iterator first,iterator last) {
		memmove(first,last,(end() - last) * sizeof(T));
		_count -= last - first;
		return first;
	}
	template<class T>
	bool vector<T>::erase_first(const T& x) {
		iterator it = find(begin(),end(),x);
		if(it != end()) {
			erase(it);
			return true;
		}
		return false;
	}
	template<class T>
	void vector<T>::swap(vector<T>& v) {
		std::swap(_elements,v._elements);
		std::swap(_size,v._size);
		std::swap(_count,v._count);
	}
	template<class T>
	void vector<T>::clear() {
		delete[] _elements;
		_size = INITIAL_SIZE;
		_elements = new T[INITIAL_SIZE];
		_count = 0;
	}

	// === freestanding operators ===
	template<class T>
	inline bool operator ==(const vector<T>& x,const vector<T>& y) {
		return compare(x,y) == 0;
	}
	template<class T>
	inline bool operator <(const vector<T>& x,const vector<T>& y) {
		return compare(x,y) < 0;
	}
	template<class T>
	inline bool operator !=(const vector<T>& x,const vector<T>& y) {
		return compare(x,y) != 0;
	}
	template<class T>
	inline bool operator >(const vector<T>& x,const vector<T>& y) {
		return compare(x,y) > 0;
	}
	template<class T>
	inline bool operator >=(const vector<T>& x,const vector<T>& y) {
		return compare(x,y) >= 0;
	}
	template<class T>
	inline bool operator <=(const vector<T>& x,const vector<T>& y) {
		return compare(x,y) <= 0;
	}

	template<class T>
	int compare(const vector<T>& x,const vector<T>& y) {
		if(x.size() != y.size())
			return x.size() - y.size();
		typename vector<T>::iterator it1,it2;
		for(it1 = x.begin(), it2 = y.begin(); it1 != x.end(); it1++, it2++) {
			if(*it1 != *it2)
				return *it1 - *it2;
		}
		return 0;
	}
	template<class T>
	void swap(vector<T>& x,vector<T>& y) {
		x.swap(y);
	}
}
