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
	class vector {
	private:
		static const u32 initialSize = 8;
		u32 elCount;
		u32 elSize;
		T *elements;

	public:
		/**
		 * Creates a new vector with space for 8 elements
		 */
		vector();
		/**
		 * Frees the space
		 */
		~vector();

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
		 * Removes the element at given index
		 *
		 * @param i the index
		 * @return true if removed
		 */
		bool remove(u32 i);

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
	vector<T>::vector() : elCount(0), elSize(initialSize) {
		elements = new T[initialSize];
	}

	template<class T>
	vector<T>::~vector() {
		delete[] elements;
	}

	template<class T>
	inline void vector<T>::add(const T &value) {
		ensureCapacity();
		elements[elCount++] = value;
	}

	template<class T>
	void vector<T>::insert(u32 i,const T &value) {
		vassert(i <= elCount,"Index %d out of bounds (0..%d)",i,elCount + 1);
		if(i == elCount)
			add(value);
		else {
			ensureCapacity();
			for(u32 x = elCount; x > i; x--)
				elements[x] = elements[x - 1];
			elements[i] = value;
			elCount++;
		}
	}

	template<class T>
	bool vector<T>::remove(u32 i) {
		if(i >= elCount)
			return false;
		for(u32 x = i + 1; x < elCount; x++)
			elements[x - 1] = elements[x];
		elCount--;
		return true;
	}

	template<class T>
	bool vector<T>::removeFirst(const T &value) {
		for(u32 i = 0; i < elCount; i++) {
			if(elements[i] == value)
				return remove(i);
		}
		return false;
	}

	template<class T>
	u32 vector<T>::removeAll(const T &value) {
		u32 c = 0;
		for(u32 i = 0; i < elCount; ) {
			if(elements[i] == value) {
				remove(i);
				c++;
				continue;
			}
			i++;
		}
		return c;
	}

	template<class T>
	inline T &vector<T>::operator[](u32 i) const {
		vassert(i < elCount,"Index %d out of bounds (0..%d)",i,elCount);
		return elements[i];
	}

	template<class T>
	inline u32 vector<T>::size() const {
		return elCount;
	}

	template<class T>
	void vector<T>::ensureCapacity() {
		if(elCount >= elSize) {
			elSize *= 2;
			T *copy = new T[elSize];
			for(u32 i = 0; i < elCount; i++)
				copy[i] = elements[i];
			delete[] elements;
			elements = copy;
		}
	}
};

#endif /* VECTOR_H_ */
