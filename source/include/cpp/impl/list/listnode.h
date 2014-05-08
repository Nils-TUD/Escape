/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#pragma once

namespace std {
	template<class T>
	class listiterator;
	template<class T>
	class const_listiterator;

	template<class T>
	class listnode {
		friend class listiterator<T>;
		friend class const_listiterator<T>;

	public:
		typedef listnode* pointer;
		typedef const listnode* const_pointer;
		typedef listnode& reference;
		typedef const listnode& const_reference;

	public:
		inline listnode()
			: _prev(nullptr), _next(nullptr), _data(T()) {
		}
		inline listnode(listnode<T> *p,listnode<T> *n)
			: _prev(p), _next(n), _data(T()) {
		}
		inline listnode(listnode<T> *p,listnode<T> *n,const T& d)
			: _prev(p), _next(n), _data(d) {
		}
		inline listnode(listnode<T> *p,listnode<T> *n,T&& d)
			: _prev(p), _next(n), _data(std::move(d)) {
		}
		inline ~listnode() {
		}

		inline listnode(const listnode<T>& l) : _prev(l.prev()), _next(l.next()), _data(l.data()) {
		}
		listnode<T>& operator=(const listnode<T>& l) {
			_prev = l.prev();
			_next = l.next();
			_data = l.data();
			return *this;
		}

		inline listnode<T>* prev() const {
			return _prev;
		}
		inline void prev(listnode<T>* p) {
			_prev = p;
		}
		inline listnode<T>* next() const {
			return _next;
		}
		inline void next(listnode<T>* n) {
			_next = n;
		}
		inline T data() const {
			return _data;
		}
		inline void data(const T& d) {
			_data = d;
		}

	private:
		listnode<T>* _prev;
		listnode<T>* _next;
		T _data;
	};
}
