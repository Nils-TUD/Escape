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

#pragma once

#include <impl/list/listnode.h>
#include <iterator>

namespace std {
	template<class T>
	class list;

	template<class T>
	class listiterator : public iterator<bidirectional_iterator_tag,T> {
		friend class list<T>;

	public:
		inline listiterator()
			: _node(), _prev() {
		}
		inline listiterator(listnode<T> *n)
			: _node(n), _prev(n->prev()) {
			assert(n != nullptr);
		}
		inline listiterator(listnode<T> *p,listnode<T> *n)
			: _node(n), _prev(p) {
		}
		inline ~listiterator() {
		}

		inline T& operator *() const {
			return _node->_data;
		}
		inline T* operator ->() const {
			return &(operator*());
		}
		inline listiterator operator +(typename listiterator<T>::difference_type n) const {
			listiterator it(_prev,_node);
			it += n;
			return it;
		}
		listiterator& operator +=(typename listiterator<T>::difference_type n) {
			while(n-- > 0) {
				_prev = _node;
				_node = _node->next();
			}
			return *this;
		}
		listiterator operator -(typename listiterator<T>::difference_type n) const {
			listiterator it(_prev,_node);
			it -= n;
			return it;
		}
		listiterator& operator -=(typename listiterator<T>::difference_type n) {
			while(n-- > 0) {
				_node = _prev;
				_prev = _prev->prev();
			}
			return *this;
		}
		inline listiterator& operator ++() {
			_prev = _node;
			_node = _node->next();
			return *this;
		}
		inline listiterator operator ++(int) {
			listiterator<T> tmp(*this);
			operator++();
			return tmp;
		}
		inline listiterator& operator --() {
			_node = _prev;
			_prev = _prev->prev();
			return *this;
		}
		inline listiterator operator --(int) {
			listiterator<T> tmp(*this);
			operator--();
			return tmp;
		}
		inline bool operator ==(const listiterator<T>& rhs) {
			return _node == rhs._node;
		}
		inline bool operator !=(const listiterator<T>& rhs) {
			return _node != rhs._node;
		}

	private:
		inline listnode<T>* node() const {
			return _node;
		}
		inline listnode<T>* prev() const {
			return _prev;
		}

	private:
		listnode<T>* _node;
		listnode<T>* _prev;
	};

	template<class T>
	class const_listiterator : public iterator<bidirectional_iterator_tag,T> {
		friend class list<T>;

	public:
		inline const_listiterator()
			: _node(), _prev() {
		}
		inline const_listiterator(const listnode<T> *n)
			: _node(n), _prev(n->prev()) {
			assert(n != nullptr);
		}
		inline const_listiterator(const listnode<T> *p,const listnode<T> *n)
			: _node(n), _prev(p) {
		}
		inline ~const_listiterator() {
		}

		inline const T& operator *() const {
			return _node->_data;
		}
		inline const T* operator ->() const {
			return &(operator*());
		}
		const_listiterator operator +(typename const_listiterator<T>::difference_type n) const {
			const_listiterator it(_prev,_node);
			it += n;
			return it;
		}
		const_listiterator& operator +=(typename const_listiterator<T>::difference_type n) {
			while(n-- > 0) {
				_prev = _node;
				_node = _node->next();
			}
			return *this;
		}
		const_listiterator operator -(typename const_listiterator<T>::difference_type n) const {
			const_listiterator it(_prev,_node);
			it -= n;
			return it;
		}
		const_listiterator& operator -=(typename const_listiterator<T>::difference_type n) {
			while(n-- > 0) {
				_node = _prev;
				_prev = _prev->prev();
			}
			return *this;
		}
		inline const_listiterator& operator ++() {
			_prev = _node;
			_node = _node->next();
			return *this;
		}
		inline const_listiterator operator ++(int) {
			const_listiterator<T> tmp(*this);
			operator++();
			return tmp;
		}
		inline const_listiterator& operator --() {
			_node = _prev;
			_prev = _prev->prev();
			return *this;
		}
		inline const_listiterator operator --(int) {
			const_listiterator<T> tmp(*this);
			operator--();
			return tmp;
		}
		inline bool operator ==(const const_listiterator<T>& rhs) {
			return _node == rhs._node;
		}
		inline bool operator !=(const const_listiterator<T>& rhs) {
			return _node != rhs._node;
		}

	private:
		inline const listnode<T>* node() const {
			return _node;
		}
		inline const listnode<T>* prev() const {
			return _prev;
		}

	private:
		const listnode<T>* _node;
		const listnode<T>* _prev;
	};
}
