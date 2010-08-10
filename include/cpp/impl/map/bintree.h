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

#ifndef BINTREE_H_
#define BINTREE_H_

#include <stddef.h>
#include <iterator>
#include <algorithm>
#include <functional>
#include <utility>
#include <limits>

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
			value_compare(Cmp c);
		public:
			bool operator ()(const value_type& x,const value_type& y) const;
		protected:
			Cmp comp;
		};

		/**
		 * Creates an empty vector
		 */
		bintree(const Cmp &compare = Cmp());
		/**
		 * Copy-constructor
		 */
		bintree(const bintree& c);
		/**
		 * Assignment-operator
		 */
		bintree& operator =(const bintree& c);
		/**
		 * Destructor
		 */
		~bintree();

		/**
		 * @return the beginning of the tree
		 */
		iterator begin();
		/**
		 * @return the beginning of the tree, as const-iterator
		 */
		const_iterator begin() const;
		/**
		 * @return the end of the tree
		 */
		iterator end();
		/**
		 * @return the end of the tree, as const-iterator
		 */
		const_iterator end() const;
		/**
		 * @return the beginning of the tree for the reverse-iterator (i.e. the end)
		 */
		reverse_iterator rbegin();
		/**
		 * @return the beginning of the tree for the const-reverse-iterator (i.e. the end)
		 */
		const_reverse_iterator rbegin() const;
		/**
		 * @return the end of the tree for the reverse-iterator (i.e. the beginning)
		 */
		reverse_iterator rend();
		/**
		 * @return the end of the tree for the const-reverse-iterator (i.e. the beginning)
		 */
		const_reverse_iterator rend() const;

		/**
		 * @return true if the tree is empty
		 */
		bool empty() const;
		/**
		 * @return the number of elements in the tree
		 */
		size_type size() const;
		/**
		 * @return the max number of elements supported
		 */
		size_type max_size() const;

		/**
		 * @return the key-compare-object
		 */
		key_compare key_comp() const;
		/**
		 * @return the value-compare-object
		 */
		value_compare value_comp() const;

		/**
		 * Inserts the key <k> with value <v> into the tree. If <k> already exists and <replace> is
		 * true, the value is replaced with <v>. Otherwise nothing is done.
		 *
		 * @param k the key
		 * @param v the value
		 * @param p the pair to insert
		 * @param replace wether the value should be replaced if the key exists
		 * @return a iterator, pointing to the inserted element
		 */
		iterator insert(const Key& k,const T& v,bool replace = true);
		iterator insert(const pair<Key,T>& p,bool replace = true);
		/**
		 * Inserts the key <k> with value <v> into the tree and gives the insert-algorithm a hint
		 * with <pos>. I.e. it tries to start at this node, if possible, which can speed up
		 * the insert a lot.
		 *
		 * @param pos the position where to start
		 * @param k the key
		 * @param v the value
		 * @param replace wether the value should be replaced if the key exists
		 * @return a iterator, pointing to the inserted element
		 */
		iterator insert(iterator pos,const Key& k,const T& v,bool replace = true);

		/**
		 * Searches for the given key and returns the position as iterator
		 *
		 * @param k the key to find
		 * @return the iterator at the position of the found key; end() if not found
		 */
		iterator find(const Key& k);
		const_iterator find(const Key& k) const;

		/**
		 * Returns an iterator to the first element that does not compare less than <x>
		 *
		 * @param x the key to find
		 * @return the iterator (end() if not found)
		 */
		iterator lower_bound(const key_type &x);
		const_iterator lower_bound(const key_type &x) const;
		/**
		 * Returns an iterator to the first element that does compare greater than <x>
		 *
		 * @param x the key to find
		 * @return the iterator (end() if not found)
		 */
		iterator upper_bound(const key_type &x);
		const_iterator upper_bound(const key_type &x) const;

		/**
		 * Removes the key-value combination with key <k>
		 *
		 * @param k the key
		 * @return true if erased
		 */
		bool erase(const Key& k);
		/**
		 * Removes the element at given position
		 *
		 * @param it the iterator that points to the element to erase
		 */
		void erase(iterator it);
		/**
		 * Removes the range [<first> .. <last>)
		 *
		 * @param first the beginning of the range (inclusive)
		 * @param last the end of the range (exclusive)
		 */
		void erase(iterator first,iterator last);
		/**
		 * Removes all elements from the tree
		 */
		void clear();

	private:
		/**
		 * Does the actual inserting, starting at <node>
		 *
		 * @param node the start-node
		 * @param k the key
		 * @param v the value
		 * @param replace wether to replace existing elements
		 * @return the insert-position
		 */
		iterator do_insert(bintree_node<Key,T,Cmp>* node,const Key& k,const T& v,bool replace);
		/**
		 * Finds the node with the minimum key in the subtree of <n>.
		 *
		 * @param n the node
		 * @return the node with the minimum key
		 */
		bintree_node<Key,T,Cmp>* find_min(bintree_node<Key,T,Cmp>* n);
		/**
		 * Replaces <n> with <newnode>. Deletes <n> when done
		 *
		 * @param n the node
		 * @param newnode the new node
		 */
		void replace_in_parent(bintree_node<Key,T,Cmp>* n,bintree_node<Key,T,Cmp>* newnode);
		/**
		 * The recursive erase-method
		 *
		 * @param n the current node
		 * @param k the key to erase
		 */
		bool do_erase(bintree_node<Key,T,Cmp>* n,const Key& k);
		/**
		 * Removes the given node
		 *
		 * @param n the node
		 */
		void do_erase(bintree_node<Key,T,Cmp>* n);

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
	bool operator ==(const bintree<Key,T,Cmp>& x,const bintree<Key,T,Cmp>& y);
	template<class Key,class T,class Cmp>
	bool operator <(const bintree<Key,T,Cmp>& x,const bintree<Key,T,Cmp>& y);
	template<class Key,class T,class Cmp>
	bool operator !=(const bintree<Key,T,Cmp>& x,const bintree<Key,T,Cmp>& y);
	template<class Key,class T,class Cmp>
	bool operator >(const bintree<Key,T,Cmp>& x,const bintree<Key,T,Cmp>& y);
	template<class Key,class T,class Cmp>
	bool operator >=(const bintree<Key,T,Cmp>& x,const bintree<Key,T,Cmp>& y);
	template<class Key,class T,class Cmp>
	bool operator <=(const bintree<Key,T,Cmp>& x,const bintree<Key,T,Cmp>& y);
}

#include "../../../../lib/cpp/impl/map/bintree.cc"

#endif /* BINTREE_H_ */
