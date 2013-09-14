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

#include <functional>
#include <utility>

namespace std {
	template<class Key,class T,class Cmp = less<Key> >
	class bintree_node {
	public:
		bintree_node()
			: _prev(nullptr), _next(nullptr), _parent(nullptr), _left(nullptr), _right(nullptr),
			  _data(make_pair<Key,T>(Key(),T())) {
		}
		bintree_node(const Key& k,bintree_node* l,bintree_node* r)
			: _prev(nullptr), _next(nullptr), _parent(nullptr), _left(l), _right(r),
			  _data(make_pair<Key,T>(k,T())) {
		}
		bintree_node(const bintree_node& c)
			: _prev(c._prev), _next(c._next), _parent(c._parent), _left(c._left),
			  _right(c._right), _data(c._data) {
		}
		bintree_node& operator =(const bintree_node& c) {
			_prev = c._prev;
			_next = c._next;
			_parent = c._parent;
			_left = c._left;
			_right = c._right;
			_data = c._data;
			return *this;
		}
		~bintree_node() {
		}

		bintree_node* parent() const {
			return _parent;
		}
		void parent(bintree_node* p) {
			_parent = p;
		}
		bintree_node* prev() const {
			return _prev;
		}
		void prev(bintree_node* p) {
			_prev = p;
		}
		bintree_node* next() const {
			return _next;
		}
		void next(bintree_node* n) {
			_next = n;
		}

		bintree_node* left() const {
			return _left;
		}
		void left(bintree_node* l) {
			_left = l;
		}
		bintree_node* right() const {
			return _right;
		}
		void right(bintree_node* r) {
			_right = r;
		}

		const pair<Key,T> &data() const {
			return _data;
		}
		const Key& key() const {
			return _data.first;
		}
		void key(const Key& k) {
			_data.first = k;
		}
		const T& value() const {
			return _data.second;
		}
		void value(const T& v) {
			_data.second = v;
		}

	private:
		bintree_node* _prev;
		bintree_node* _next;
		bintree_node* _parent;
		bintree_node* _left;
		bintree_node* _right;
		pair<Key,T> _data;
	};
}
