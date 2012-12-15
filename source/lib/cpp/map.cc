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
	// === constructors/destructors ===
	template<class Key,class T,class Cmp>
	inline map<Key,T,Cmp>::map(const Cmp& comp)
		: _tree(bintree<Key,T,Cmp>(comp)) {
	}
	template<class Key,class T,class Cmp>
	template<class InputIterator>
	map<Key,T,Cmp>::map(InputIterator first,InputIterator last,const Cmp& comp)
		: _tree(bintree<Key,T,Cmp>(comp)) {
		for(; first != last; ++first)
			insert(*first);
	}
	template<class Key,class T,class Cmp>
	inline map<Key,T,Cmp>::map(const map<Key,T,Cmp>& x)
		: _tree(x._tree) {
	}
	template<class Key,class T,class Cmp>
	inline map<Key,T,Cmp>::~map() {
	}
	template<class Key,class T,class Cmp>
	inline map<Key,T,Cmp>& map<Key,T,Cmp>::operator =(const map<Key,T,Cmp>& x) {
		_tree = x._tree;
		return *this;
	}

	// === iterators ===
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::iterator map<Key,T,Cmp>::begin() {
		return _tree.begin();
	}
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::const_iterator map<Key,T,Cmp>::begin() const {
		return _tree.begin();
	}
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::iterator map<Key,T,Cmp>::end() {
		return _tree.end();
	}
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::const_iterator map<Key,T,Cmp>::end() const {
		return _tree.end();
	}
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::reverse_iterator map<Key,T,Cmp>::rbegin() {
		return _tree.rbegin();
	}
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::const_reverse_iterator map<Key,T,Cmp>::rbegin() const {
		return _tree.rbegin();
	}
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::reverse_iterator map<Key,T,Cmp>::rend() {
		return _tree.rend();
	}
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::const_reverse_iterator map<Key,T,Cmp>::rend() const {
		return _tree.rend();
	}

	// === sizes ===
	template<class Key,class T,class Cmp>
	inline bool map<Key,T,Cmp>::empty() const {
		return _tree.empty();
	}
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::size_type map<Key,T,Cmp>::size() const {
		return _tree.size();
	}
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::size_type map<Key,T,Cmp>::max_size() const {
		return _tree.max_size();
	}

	// === operator[] and at() ===
	template<class Key,class T,class Cmp>
	T& map<Key,T,Cmp>::operator [](const key_type& x) {
		iterator it = _tree.find(x);
		if(it == _tree.end())
			it = _tree.insert(x,T());
		return it->second;
	}
	template<class Key,class T,class Cmp>
	T& map<Key,T,Cmp>::at(const key_type& x) {
		iterator it = _tree.find(x);
		if(it == _tree.end())
			throw out_of_range("Key not found");
		return it->second;
	}
	template<class Key,class T,class Cmp>
	const T& map<Key,T,Cmp>::at(const key_type& x) const {
		iterator it = _tree.find(x);
		if(it == _tree.end())
			throw out_of_range("Key not found");
		return it->second;
	}

	// === insert ===
	template<class Key,class T,class Cmp>
	pair<typename map<Key,T,Cmp>::iterator,bool> map<Key,T,Cmp>::insert(const value_type& x) {
		size_type s = _tree.size();
		iterator it = _tree.insert(x,false);
		return make_pair<iterator,bool>(it,_tree.size() > s);
	}
	template<class Key,class T,class Cmp>
	typename map<Key,T,Cmp>::iterator map<Key,T,Cmp>::insert(iterator pos,const value_type& x) {
		return _tree.insert(pos,x.first,x.second,false);
	}
	template<class Key,class T,class Cmp>
	template<class InputIterator>
	void map<Key,T,Cmp>::insert(InputIterator first,InputIterator last) {
		for(; first != last; ++first)
			insert(*first);
	}

	// === erase ===
	template<class Key,class T,class Cmp>
	inline void map<Key,T,Cmp>::erase(iterator position) {
		_tree.erase(position);
	}
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::size_type map<Key,T,Cmp>::erase(const key_type& x) {
		return _tree.erase(x) ? 1 : 0;
	}
	template<class Key,class T,class Cmp>
	inline void map<Key,T,Cmp>::erase(iterator first,iterator last) {
		_tree.erase(first,last);
	}
	template<class Key,class T,class Cmp>
	inline void map<Key,T,Cmp>::swap(map<Key,T,Cmp>& x) {
		std::swap(*this,x);
	}
	template<class Key,class T,class Cmp>
	inline void map<Key,T,Cmp>::clear() {
		_tree.clear();
	}

	// === comparation-objects ===
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::key_compare map<Key,T,Cmp>::key_comp() const {
		return _tree.key_comp();
	}
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::value_compare map<Key,T,Cmp>::value_comp() const {
		return _tree.value_comp();
	}

	// === find ===
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::iterator map<Key,T,Cmp>::find(const key_type& x) {
		return _tree.find(x);
	}
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::const_iterator map<Key,T,Cmp>::find(const key_type& x) const {
		return _tree.find(x);
	}
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::size_type map<Key,T,Cmp>::count(const key_type& x) const {
		return _tree.find(x) == end() ? 0 : 1;
	}

	// === bounds ===
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::iterator map<Key,T,Cmp>::lower_bound(const key_type &x) {
		return _tree.lower_bound(x);
	}
	template<class Key,class T,class Cmp>
	inline typename map<Key,T,Cmp>::const_iterator map<Key,T,Cmp>::lower_bound(
			const key_type &x) const {
		return _tree.lower_bound(x);
	}
	template<class Key,class T,class Cmp>
	typename map<Key,T,Cmp>::iterator map<Key,T,Cmp>::upper_bound(const key_type &x) {
		return _tree.upper_bound(x);
	}
	template<class Key,class T,class Cmp>
	typename map<Key,T,Cmp>::const_iterator map<Key,T,Cmp>::upper_bound(const key_type &x) const {
		return _tree.upper_bound(x);
	}
	template<class Key,class T,class Cmp>
	pair<typename map<Key,T,Cmp>::iterator,typename map<Key,T,Cmp>::iterator>
		map<Key,T,Cmp>::equal_range(const key_type& x) {
		return make_pair<iterator,iterator>(lower_bound(x),upper_bound(x));
	}
	template<class Key,class T,class Cmp>
	pair<typename map<Key,T,Cmp>::const_iterator,typename map<Key,T,Cmp>::const_iterator>
		map<Key,T,Cmp>::equal_range(const key_type& x) const {
		return make_pair<const_iterator,const_iterator>(lower_bound(x),upper_bound(x));
	}

	// === global operators ===
	template<class Key,class T,class Cmp>
	inline bool operator ==(const map<Key,T,Cmp>& x,const map<Key,T,Cmp>& y) {
		return x._tree == y._tree;
	}
	template<class Key,class T,class Cmp>
	inline bool operator <(const map<Key,T,Cmp>& x,const map<Key,T,Cmp>& y) {
		return x._tree < y._tree;
	}
	template<class Key,class T,class Cmp>
	inline bool operator !=(const map<Key,T,Cmp>& x,const map<Key,T,Cmp>& y) {
		return !(x == y);
	}
	template<class Key,class T,class Cmp>
	inline bool operator >(const map<Key,T,Cmp>& x,const map<Key,T,Cmp>& y) {
		return y < x;
	}
	template<class Key,class T,class Cmp>
	inline bool operator >=(const map<Key,T,Cmp>& x,const map<Key,T,Cmp>& y) {
		return !(x < y);
	}
	template<class Key,class T,class Cmp>
	inline bool operator <=(const map<Key,T,Cmp>& x,const map<Key,T,Cmp>& y) {
		return !(y < x);
	}

	template<class Key,class T,class Cmp>
	void swap(map<Key,T,Cmp>& x,map<Key,T,Cmp>& y) {
		x.swap(y);
	}
}
