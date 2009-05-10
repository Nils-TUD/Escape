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

#ifndef VECTOR_H_
#define VECTOR_H_

#include <esc/common.h>
#include <string.h>
#include <assert.h>

namespace esc {
	/**
	 * An automatically increased array for type <T>. Initially it has space for 8
	 * elements. As soon as there is no free slot in the array anymore, more space is allocated.
	 */
	template<class T>
	class Vector {
	private:
		static const u32 initialSize = 8;
		u32 _elCount;
		u32 _elSize;
		T *_elements;

	public:
		/**
		 * Creates a new vector with space for <size> elements
		 *
		 * @param vsize the initial size (8 by default)
		 */
		Vector(u32 vsize = initialSize);
		/**
		 * Copy-constructor
		 */
		Vector(const Vector &v);
		/**
		 * Frees the space
		 */
		~Vector();

		/**
		 * Assignment-operator
		 */
		Vector<T> &operator=(const Vector<T> &v);

		/**
		 * Adds the given value to the end of the vector
		 *
		 * @param value the value
		 */
		void add(const T &value);

		/**
		 * Inserts <value> at <i>. Note that <i> has to be between 0 and size()
		 *
		 * @param i the index
		 * @param value the value
		 */
		void insert(u32 i,const T &value);

		/**
		 * Removes elements beginning at given index
		 *
		 * @param i the index
		 * @param count the number of elements to remove
		 * @return the number of removed elements
		 */
		u32 remove(u32 i,u32 count = 1);

		/**
		 * Removes the first element that equals <value>
		 *
		 * @param value the value to remove
		 * @return true if removed
		 */
		bool removeFirst(const T &value);

		/**
		 * Removes all values that equal <value>
		 *
		 * @param value the value to remove
		 * @return the number of removed elements
		 */
		u32 removeAll(const T &value);

		/**
		 * @param i the index
		 * @return element at index i
		 */
		T &operator[](u32 i) const;

		/**
		 * @return the number of elements
		 */
		u32 size() const;

	private:
		void ensureCapacity();
	};

	template<class T>
	Vector<T>::Vector(u32 vsize) : _elCount(0), _elSize(vsize) {
		_elements = new T[vsize];
	}

	template<class T>
	Vector<T>::Vector(const Vector<T> &v) {
		_elements = new T[v._elSize];
		memcpy(_elements,v._elements,v._elCount * sizeof(T));
		_elCount = v._elCount;
		_elSize = v._elSize;
	}

	template<class T>
	Vector<T>::~Vector() {
		delete[] _elements;
	}

	template<class T>
	Vector<T> &Vector<T>::operator=(const Vector<T> &v) {
		// ignore self-assignments
		if(this == &v)
			return *this;
		_elements = new T[v._elSize];
		memcpy(_elements,v._elements,v._elCount * sizeof(T));
		_elCount = v._elCount;
		_elSize = v._elSize;
		return *this;
	}

	template<class T>
	inline void Vector<T>::add(const T &value) {
		ensureCapacity();
		_elements[_elCount++] = value;
	}

	template<class T>
	void Vector<T>::insert(u32 i,const T &value) {
		vassert(i <= _elCount,"Index %d out of bounds (0..%d)",i,_elCount);
		if(i == _elCount)
			add(value);
		else {
			ensureCapacity();
			for(u32 x = _elCount; x > i; x--)
				_elements[x] = _elements[x - 1];
			_elements[i] = value;
			_elCount++;
		}
	}

	template<class T>
	u32 Vector<T>::remove(u32 i,u32 count) {
		vassert(i + count <= _elCount,"Range %d..%d out of bounds (0..%d)",i,i + count,_elCount);
		for(u32 x = i + count; x < _elCount; x++)
			_elements[x - count] = _elements[x];
		_elCount -= count;
		return count;
	}

	template<class T>
	bool Vector<T>::removeFirst(const T &value) {
		for(u32 i = 0; i < _elCount; i++) {
			if(_elements[i] == value)
				return remove(i);
		}
		return false;
	}

	template<class T>
	u32 Vector<T>::removeAll(const T &value) {
		u32 c = 0;
		for(u32 i = 0; i < _elCount; ) {
			if(_elements[i] == value) {
				remove(i);
				c++;
				continue;
			}
			i++;
		}
		return c;
	}

	template<class T>
	inline T &Vector<T>::operator[](u32 i) const {
		vassert(i < _elCount,"Index %d out of bounds (0..%d)",i,_elCount);
		return _elements[i];
	}

	template<class T>
	inline u32 Vector<T>::size() const {
		return _elCount;
	}

	template<class T>
	void Vector<T>::ensureCapacity() {
		if(_elCount >= _elSize) {
			_elSize *= 2;
			T *copy = new T[_elSize];
			for(u32 i = 0; i < _elCount; i++)
				copy[i] = _elements[i];
			delete[] _elements;
			_elements = copy;
		}
	}
};

#endif /* VECTOR_H_ */
