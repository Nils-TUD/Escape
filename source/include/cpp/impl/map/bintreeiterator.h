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

#include <impl/map/bintreenode.h>
#include <functional>
#include <iterator>
#include <utility>

namespace std {
	template<class Key,class T,class Cmp>
	class bintree;

	template<class Key,class T,class Cmp = less<Key> >
	class bintree_iterator : public iterator<bidirectional_iterator_tag,pair<Key,T> > {
		friend class bintree<Key,T,Cmp>;
	public:
		bintree_iterator()
			: _node(nullptr) {
		}
		bintree_iterator(bintree_node<Key,T,Cmp> *n)
			: _node(n) {
		}
		~bintree_iterator() {
		}

		pair<Key,T>& operator *() const {
			// TODO const?
			return const_cast<pair<Key,T>&>(_node->data());
		}
		pair<Key,T>* operator ->() const {
			return &(operator*());
		}
		bintree_iterator& operator ++() {
			_node = _node->next();
			return *this;
		}
		bintree_iterator operator ++(int) {
			bintree_iterator<Key,T,Cmp> tmp(*this);
			operator++();
			return tmp;
		}
		bintree_iterator& operator --() {
			_node = _node->prev();
			return *this;
		}
		bintree_iterator operator --(int) {
			bintree_iterator<Key,T,Cmp> tmp(*this);
			operator--();
			return tmp;
		}
		bool operator ==(const bintree_iterator<Key,T,Cmp>& rhs) {
			return _node == rhs._node;
		}
		bool operator !=(const bintree_iterator<Key,T,Cmp>& rhs) {
			return _node != rhs._node;
		}

	private:
		bintree_node<Key,T,Cmp>* node() const {
			return _node;
		}

	private:
		bintree_node<Key,T,Cmp>* _node;
	};

	// === const-iterator ===
	template<class Key,class T,class Cmp = less<Key> >
	class const_bintree_iterator : public iterator<bidirectional_iterator_tag,pair<Key,T> > {
		friend class bintree<Key,T,Cmp>;
	public:
		const_bintree_iterator()
			: _node(nullptr) {
		}
		const_bintree_iterator(const bintree_node<Key,T,Cmp> *n)
			: _node(n) {
		}
		~const_bintree_iterator() {
		}

		const pair<Key,T>& operator *() const {
			return _node->data();
		}
		const pair<Key,T>* operator ->() const {
			return &(operator*());
		}
		const_bintree_iterator& operator ++() {
			_node = _node->next();
			return *this;
		}
		const_bintree_iterator operator ++(int) {
			const_bintree_iterator<Key,T,Cmp> tmp(*this);
			operator++();
			return tmp;
		}
		const_bintree_iterator& operator --() {
			_node = _node->prev();
			return *this;
		}
		const_bintree_iterator operator --(int) {
			const_bintree_iterator<Key,T,Cmp> tmp(*this);
			operator--();
			return tmp;
		}
		bool operator ==(const const_bintree_iterator<Key,T,Cmp>& rhs) {
			return _node == rhs._node;
		}
		bool operator !=(const const_bintree_iterator<Key,T,Cmp>& rhs) {
			return _node != rhs._node;
		}

	private:
		const bintree_node<Key,T,Cmp>* node() const {
			return _node;
		}

	private:
		const bintree_node<Key,T,Cmp>* _node;
	};
}
