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

#include <impl/map/bintreeiterator.h>
#include <impl/map/bintreenode.h>
#include <bits/c++config.h>
#include <algorithm>
#include <functional>
#include <iterator>
#include <limits>
#include <stddef.h>
#include <utility>

// Note: algorithms are based on http://en.wikipedia.org/wiki/Binary_search_tree

namespace std {
	template<class Key,class T,class Cmp>
	class bintree_node;
	template<class Key,class T,class Cmp>
	class bintree_iterator;
	template<class Key,class T,class Cmp>
	class const_bintree_iterator;

	/**
	 * A binary search tree with sorted keys (defined by the compare-object). This is used for
	 * the map-implementation.
	 */
	template<class Key,class T,class Cmp = less<Key> >
	class bintree {
	public:
		typedef Key key_type;
		typedef T mapped_type;
		typedef pair<const Key,T> value_type;
		typedef Cmp key_compare;
		typedef T& reference;
		typedef const T& const_reference;
		typedef bintree_iterator<Key,T,Cmp> iterator;
		typedef const_bintree_iterator<Key,T,Cmp> const_iterator;
		typedef size_t size_type;
		typedef long difference_type;
		typedef T* pointer;
		typedef const T* const_pointer;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	public:
		/**
		 * The value-compare-object
		 */
		class value_compare: public binary_function<value_type,value_type,bool> {
			friend class bintree;
		protected:
			value_compare(Cmp c)
				: comp(c) {
			}
		public:
			bool operator ()(const value_type& x,const value_type& y) const {
				return comp(x.first,y.first);
			}
		protected:
			Cmp comp;
		};

		/**
		 * Creates an empty vector
		 */
		bintree(const Cmp &compare = Cmp())
			: _cmp(compare), _elCount(0), _head(bintree_node<Key,T,Cmp>()),
			  _foot(bintree_node<Key,T,Cmp>()) {
			_head.next(&_foot);
			_foot.prev(&_head);
		}
		/**
		 * Copy-constructor
		 */
		bintree(const bintree& c)
			: _cmp(c._cmp), _elCount(0), _head(bintree_node<Key,T,Cmp>()),
			  _foot(bintree_node<Key,T,Cmp>()) {
			_head.next(&_foot);
			_foot.prev(&_head);
			for(const_iterator it = c.begin(); it != c.end(); ++it)
				insert(it->first,it->second);
		}
		/**
		 * Assignment-operator
		 */
		bintree& operator =(const bintree& c) {
			clear();
			_cmp = c._cmp;
			for(const_iterator it = c.begin(); it != c.end(); ++it)
				insert(it->first,it->second);
			return *this;
		}
		/**
		 * Destructor
		 */
		~bintree() {
			clear();
		}

		/**
		 * @return the beginning of the tree
		 */
		iterator begin() {
			return iterator(_head.next());
		}
		/**
		 * @return the beginning of the tree, as const-iterator
		 */
		const_iterator begin() const {
			return const_iterator(_head.next());
		}
		/**
		 * @return the end of the tree
		 */
		iterator end() {
			return iterator(&_foot);
		}
		/**
		 * @return the end of the tree, as const-iterator
		 */
		const_iterator end() const {
			return const_iterator(&_foot);
		}
		/**
		 * @return the beginning of the tree for the reverse-iterator (i.e. the end)
		 */
		reverse_iterator rbegin() {
			return reverse_iterator(&_foot);
		}
		/**
		 * @return the beginning of the tree for the const-reverse-iterator (i.e. the end)
		 */
		const_reverse_iterator rbegin() const {
			return const_reverse_iterator(&_foot);
		}
		/**
		 * @return the end of the tree for the reverse-iterator (i.e. the beginning)
		 */
		reverse_iterator rend() {
			return reverse_iterator(_head.next());
		}
		/**
		 * @return the end of the tree for the const-reverse-iterator (i.e. the beginning)
		 */
		const_reverse_iterator rend() const {
			return const_reverse_iterator(_head.next());
		}

		/**
		 * @return true if the tree is empty
		 */
		bool empty() const {
			return _elCount == 0;
		}
		/**
		 * @return the number of elements in the tree
		 */
		size_type size() const {
			return _elCount;
		}
		/**
		 * @return the max number of elements supported
		 */
		size_type max_size() const {
			// TODO ??
			return numeric_limits<size_type>::max();
		}

		/**
		 * @return the key-compare-object
		 */
		key_compare key_comp() const {
			return _cmp;
		}
		/**
		 * @return the value-compare-object
		 */
		value_compare value_comp() const {
			return value_compare(key_comp());
		}

		/**
		 * Inserts the key <k> with value <v> into the tree. If <k> already exists and <replace> is
		 * true, the value is replaced with <v>. Otherwise nothing is done.
		 *
		 * @param k the key
		 * @param v the value
		 * @param p the pair to insert
		 * @param replace whether the value should be replaced if the key exists
		 * @return a iterator, pointing to the inserted element
		 */
		iterator insert(const Key& k,const T& v,bool replace = true) {
			return do_insert(_head.right(),k,v,replace);
		}
		iterator insert(const pair<Key,T>& p,bool replace = true) {
			return insert(p.first,p.second,replace);
		}
		/**
		 * Inserts the key <k> with value <v> into the tree and gives the insert-algorithm a hint
		 * with <pos>. I.e. it tries to start at this node, if possible, which can speed up
		 * the insert a lot.
		 *
		 * @param pos the position where to start
		 * @param k the key
		 * @param v the value
		 * @param replace whether the value should be replaced if the key exists
		 * @return a iterator, pointing to the inserted element
		 */
		iterator insert(iterator pos,const Key& k,const T& v,bool replace = true) {
			bintree_node<Key,T,Cmp>* node = pos.node();
			// head and foot are not allowed
			if(node == &_head || node == &_foot)
				node = _head.right();
			else {
				// while the parent is less than k, move upwards
				while(node->parent() && _cmp(k,node->parent()->key()))
					node = node->parent();
			}
			return do_insert(node,k,v,replace);
		}

		/**
		 * Searches for the given key and returns the position as iterator
		 *
		 * @param k the key to find
		 * @return the iterator at the position of the found key; end() if not found
		 */
		iterator find(const Key& k) {
			bintree_node<Key,T,Cmp>* node = _head.right();
			while(node != nullptr) {
				// less?
				if(_cmp(k,node->key()))
					node = node->left();
				else if(k == node->key())
					return iterator(node);
				else
					node = node->right();
			}
			// not found
			return end();
		}
		const_iterator find(const Key& k) const {
			iterator it = const_cast<bintree*>(this)->find(k);
			return const_iterator(it.node());
		}

		/**
		 * Returns an iterator to the first element that does not compare less than <x>
		 *
		 * @param x the key to find
		 * @return the iterator (end() if not found)
		 */
		iterator lower_bound(const key_type &x) {
			bintree_node<Key,T,Cmp>* node = _head.right();
			while(node != nullptr) {
				if(_cmp(x,node->key())) {
					if(!node->left() || !_cmp(x,node->left()->key()))
						return iterator(node);
					node = node->left();
				}
				else if(x == node->key())
					return iterator(node);
				else
					node = node->right();
			}
			return end();
		}
		const_iterator lower_bound(const key_type &x) const {
			iterator it = lower_bound(x);
			return const_iterator(it.node());
		}
		/**
		 * Returns an iterator to the first element that does compare greater than <x>
		 *
		 * @param x the key to find
		 * @return the iterator (end() if not found)
		 */
		iterator upper_bound(const key_type &x) {
			bintree_node<Key,T,Cmp>* node = _head.right();
			while(node != nullptr) {
				if(_cmp(x,node->key())) {
					if(!node->left() || !_cmp(x,node->left()->key()))
						return iterator(node);
					node = node->left();
				}
				else if(x == node->key())
					return iterator(node->next());
				else
					node = node->right();
			}
			return end();
		}
		const_iterator upper_bound(const key_type &x) const {
			iterator it = upper_bound(x);
			return const_iterator(it.node());
		}

		/**
		 * Removes the key-value combination with key <k>
		 *
		 * @param k the key
		 * @return true if erased
		 */
		bool erase(const Key& k) {
			// don't even try it when _elCount is 0
			if(_elCount > 0)
				return do_erase(_head.right(),k);
			return false;
		}
		/**
		 * Removes the element at given position
		 *
		 * @param it the iterator that points to the element to erase
		 */
		void erase(iterator it) {
			bintree_node<Key,T,Cmp>* node = it.node();
			do_erase(node);
		}
		/**
		 * Removes the range [<first> .. <last>)
		 *
		 * @param first the beginning of the range (inclusive)
		 * @param last the end of the range (exclusive)
		 */
		void erase(iterator first,iterator last) {
			/* note that we have to run to the node each time because deleted nodes might be reused for
			 * other nodes. that is, we can't simply iterator through the sequence and delete every
			 * node. doing so might free nodes twice. */
			difference_type i = distance(begin(),first);
			difference_type num = distance(first,last);
			while(num-- > 0) {
				iterator it = begin();
				advance(it,i);
				do_erase(it.node());
			}
		}
		/**
		 * Removes all elements from the tree
		 */
		void clear() {
			bintree_node<Key,T,Cmp>* node = _head.next();
			while(node != &_foot) {
				bintree_node<Key,T,Cmp>* next = node->next();
				delete node;
				node = next;
			}
			_elCount = 0;
			_head.right(nullptr);
			_head.next(&_foot);
			_foot.prev(&_head);
		}

	private:
		/**
		 * Does the actual inserting, starting at <node>
		 *
		 * @param node the start-node
		 * @param k the key
		 * @param v the value
		 * @param replace whether to replace existing elements
		 * @return the insert-position
		 */
		iterator do_insert(bintree_node<Key,T,Cmp>* node,const Key& k,const T& v,bool replace) {
			bool left = false;
			bintree_node<Key,T,Cmp>* prev = node ? node->parent() : nullptr;
			while(node != nullptr) {
				prev = node;
				// less?
				if(_cmp(k,node->key())) {
					left = true;
					node = node->left();
				}
				// equal, so just replace the value
				else if(k == node->key()) {
					if(replace)
						node->value(v);
					return iterator(node);
				}
				else {
					left = false;
					node = node->right();
				}
			}

			// node is null, so create a new node
			node = new bintree_node<Key,T,Cmp>(k,nullptr,nullptr);
			node->value(v);
			if(!prev)
				prev = &_head;
			node->parent(prev);

			// insert into tree
			if(left)
				prev->left(node);
			else
				prev->right(node);

			// insert into sequence
			// note that its always directly behind or before the found node when we want to keep
			// the keys in ascending order!
			if(left) {
				// insert before prev
				prev->prev()->next(node);
				node->prev(prev->prev());
				prev->prev(node);
				node->next(prev);
			}
			else {
				// insert behind prev
				node->next(prev->next());
				node->prev(prev);
				prev->next()->prev(node);
				prev->next(node);
			}

			_elCount++;
			return iterator(node);
		}
		/**
		 * Finds the node with the minimum key in the subtree of <n>.
		 *
		 * @param n the node
		 * @return the node with the minimum key
		 */
		bintree_node<Key,T,Cmp>* find_min(bintree_node<Key,T,Cmp>* n) {
			bintree_node<Key,T,Cmp>* current = n;
			while(current->left())
				current = current->left();
			return current;
		}
		/**
		 * Replaces <n> with <newnode>. Deletes <n> when done
		 *
		 * @param n the node
		 * @param newnode the new node
		 */
		void replace_in_parent(bintree_node<Key,T,Cmp>* n,bintree_node<Key,T,Cmp>* newnode) {
			bintree_node<Key,T,Cmp>* parent = n->parent();
			// erase out of the sequence
			n->prev()->next(n->next());
			n->next()->prev(n->prev());
			// replace in parent
			if(parent) {
				if(n == parent->left())
					parent->left(newnode);
				else
					parent->right(newnode);
			}
			if(newnode)
				newnode->parent(parent);
			delete n;
		}
		/**
		 * The recursive erase-method
		 *
		 * @param n the current node
		 * @param k the key to erase
		 */
		bool do_erase(bintree_node<Key,T,Cmp>* n,const Key& k) {
			if(n) {
				if(_cmp(k,n->key()))
					return do_erase(n->left(),k);
				else if(k != n->key())
					return do_erase(n->right(),k);
				else {
					do_erase(n);
					return true;
				}
			}
			return false;
		}
		/**
		 * Removes the given node
		 *
		 * @param n the node
		 */
		void do_erase(bintree_node<Key,T,Cmp>* n) {
			if(n->left() && n->right()) {
				bintree_node<Key,T,Cmp>* successor = find_min(n->right());
				n->key(successor->key());
				n->value(successor->value());
				replace_in_parent(successor,successor->right());
			}
			else if(n->left() || n->right()) {
				if(n->left())
					replace_in_parent(n,n->left());
				else
					replace_in_parent(n,n->right());
			}
			else
				replace_in_parent(n,nullptr);
			_elCount--;
		}

	private:
		Cmp _cmp;
		size_type _elCount;
		bintree_node<Key,T,Cmp> _head;
		bintree_node<Key,T,Cmp> _foot;
	};

	/**
	 * Comparison-operators based on std::lexigraphical_compare and std::equal
	 */
	template<class Key,class T,class Cmp>
	inline bool operator ==(const bintree<Key,T,Cmp>& x,const bintree<Key,T,Cmp>& y) {
		return x.size() == y.size() && std::equal(x.begin(),x.end(),y.begin());
	}
	template<class Key,class T,class Cmp>
	inline bool operator <(const bintree<Key,T,Cmp>& x,const bintree<Key,T,Cmp>& y) {
		return std::lexicographical_compare(x.begin(),x.end(),y.begin(),y.end(),x.value_comp());
	}
	template<class Key,class T,class Cmp>
	inline bool operator !=(const bintree<Key,T,Cmp>& x,const bintree<Key,T,Cmp>& y) {
		return !(x == y);
	}
	template<class Key,class T,class Cmp>
	inline bool operator >(const bintree<Key,T,Cmp>& x,const bintree<Key,T,Cmp>& y) {
		return y < x;
	}
	template<class Key,class T,class Cmp>
	inline bool operator >=(const bintree<Key,T,Cmp>& x,const bintree<Key,T,Cmp>& y) {
		return !(x < y);
	}
	template<class Key,class T,class Cmp>
	inline bool operator <=(const bintree<Key,T,Cmp>& x,const bintree<Key,T,Cmp>& y) {
		return !(y < x);
	}
}
