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

// Note: algorithms are based on http://en.wikipedia.org/wiki/Binary_search_tree

namespace std {
	// === value_compare ===
	template<class Key,class T,class Cmp>
	inline bintree<Key,T,Cmp>::value_compare::value_compare(Cmp c)
		: comp(c) {
	}
	template<class Key,class T,class Cmp>
	inline bool bintree<Key,T,Cmp>::value_compare::operator ()(const value_type& x,const value_type& y)
			const {
		return comp(x.first,y.first);
	}

	// === bintree_node ===
	template<class Key,class T,class Cmp = less<Key> >
	class bintree_node {
	public:
		bintree_node()
			: _prev(NULL), _next(NULL), _parent(NULL), _left(NULL), _right(NULL),
			  _data(make_pair<Key,T>(Key(),T())) {
		}
		bintree_node(const Key& k,bintree_node* l,bintree_node* r)
			: _prev(NULL), _next(NULL), _parent(NULL), _left(l), _right(r),
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

	// === iterator ===
	template<class Key,class T,class Cmp = less<Key> >
	class bintree_iterator : public iterator<bidirectional_iterator_tag,pair<Key,T> > {
		friend class bintree<Key,T,Cmp>;
	public:
		bintree_iterator()
			: _node(NULL) {
		};
		bintree_iterator(bintree_node<Key,T,Cmp> *n)
			: _node(n) {
		};
		~bintree_iterator() {
		};

		pair<Key,T>& operator *() const {
			// TODO const?
			return const_cast<pair<Key,T>&>(_node->data());
		};
		pair<Key,T>* operator ->() const {
			return &(operator*());
		};
		bintree_iterator& operator ++() {
			_node = _node->next();
			return *this;
		};
		bintree_iterator operator ++(int) {
			bintree_iterator<Key,T,Cmp> tmp(*this);
			operator++();
			return tmp;
		};
		bintree_iterator& operator --() {
			_node = _node->prev();
			return *this;
		};
		bintree_iterator operator --(int) {
			bintree_iterator<Key,T,Cmp> tmp(*this);
			operator--();
			return tmp;
		};
		bool operator ==(const bintree_iterator<Key,T,Cmp>& rhs) {
			return _node == rhs._node;
		};
		bool operator !=(const bintree_iterator<Key,T,Cmp>& rhs) {
			return _node != rhs._node;
		};

	private:
		bintree_node<Key,T,Cmp>* node() const {
			return _node;
		};

	private:
		bintree_node<Key,T,Cmp>* _node;
	};

	// === const-iterator ===
	template<class Key,class T,class Cmp = less<Key> >
	class const_bintree_iterator : public iterator<bidirectional_iterator_tag,pair<Key,T> > {
		friend class bintree<Key,T,Cmp>;
	public:
		const_bintree_iterator()
			: _node(NULL) {
		};
		const_bintree_iterator(const bintree_node<Key,T,Cmp> *n)
			: _node(n) {
		};
		~const_bintree_iterator() {
		};

		const pair<Key,T>& operator *() const {
			return _node->data();
		};
		const pair<Key,T>* operator ->() const {
			return &(operator*());
		};
		const_bintree_iterator& operator ++() {
			_node = _node->next();
			return *this;
		};
		const_bintree_iterator operator ++(int) {
			const_bintree_iterator<Key,T,Cmp> tmp(*this);
			operator++();
			return tmp;
		};
		const_bintree_iterator& operator --() {
			_node = _node->prev();
			return *this;
		};
		const_bintree_iterator operator --(int) {
			const_bintree_iterator<Key,T,Cmp> tmp(*this);
			operator--();
			return tmp;
		};
		bool operator ==(const const_bintree_iterator<Key,T,Cmp>& rhs) {
			return _node == rhs._node;
		};
		bool operator !=(const const_bintree_iterator<Key,T,Cmp>& rhs) {
			return _node != rhs._node;
		};

	private:
		const bintree_node<Key,T,Cmp>* node() const {
			return _node;
		};

	private:
		const bintree_node<Key,T,Cmp>* _node;
	};

	// === constructors/destructors ===
	template<class Key,class T,class Cmp>
	bintree<Key,T,Cmp>::bintree(const Cmp &compare)
		: _cmp(compare), _elCount(0), _head(bintree_node<Key,T,Cmp>()),
		  _foot(bintree_node<Key,T,Cmp>()) {
		_head.next(&_foot);
		_foot.prev(&_head);
	}
	template<class Key,class T,class Cmp>
	bintree<Key,T,Cmp>::bintree(const bintree<Key,T,Cmp>& c)
		: _cmp(c._cmp), _elCount(0), _head(bintree_node<Key,T,Cmp>()),
		  _foot(bintree_node<Key,T,Cmp>()) {
		_head.next(&_foot);
		_foot.prev(&_head);
		for(const_iterator it = c.begin(); it != c.end(); ++it)
			insert(it->first,it->second);
	}
	template<class Key,class T,class Cmp>
	bintree<Key,T,Cmp>& bintree<Key,T,Cmp>::operator =(const bintree<Key,T,Cmp>& c) {
		clear();
		_cmp = c._cmp;
		for(const_iterator it = c.begin(); it != c.end(); ++it)
			insert(it->first,it->second);
		return *this;
	}
	template<class Key,class T,class Cmp>
	inline bintree<Key,T,Cmp>::~bintree() {
		clear();
	}

	// === sizes ===
	template<class Key,class T,class Cmp>
	inline bool bintree<Key,T,Cmp>::empty() const {
		return _elCount == 0;
	}
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::size_type bintree<Key,T,Cmp>::size() const {
		return _elCount;
	}
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::size_type bintree<Key,T,Cmp>::max_size() const {
		// TODO ??
		return numeric_limits<size_type>::max();
	}

	// === comparation-object ===
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::key_compare bintree<Key,T,Cmp>::key_comp() const {
		return _cmp;
	}
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::value_compare bintree<Key,T,Cmp>::value_comp() const {
		return value_compare(key_comp());
	}

	// === iterator stuff ===
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::iterator bintree<Key,T,Cmp>::begin() {
		return iterator(_head.next());
	}
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::const_iterator bintree<Key,T,Cmp>::begin() const {
		return const_iterator(_head.next());
	}
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::iterator bintree<Key,T,Cmp>::end() {
		return iterator(&_foot);
	}
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::const_iterator bintree<Key,T,Cmp>::end() const {
		return const_iterator(&_foot);
	}
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::reverse_iterator bintree<Key,T,Cmp>::rbegin() {
		return reverse_iterator(&_foot);
	}
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::const_reverse_iterator bintree<Key,T,Cmp>::rbegin() const {
		return const_reverse_iterator(&_foot);
	}
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::reverse_iterator bintree<Key,T,Cmp>::rend() {
		return reverse_iterator(_head.next());
	}
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::const_reverse_iterator bintree<Key,T,Cmp>::rend() const {
		return const_reverse_iterator(_head.next());
	}

	// === manipulation ===
	template<class Key,class T,class Cmp>
	typename bintree<Key,T,Cmp>::iterator bintree<Key,T,Cmp>::insert(const Key& k,const T& v,
			bool replace) {
		bool left = false;
		bintree_node<Key,T,Cmp>* prev = NULL;
		bintree_node<Key,T,Cmp>* node = _head.right();
		while(node != NULL) {
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
		node = new bintree_node<Key,T,Cmp>(k,NULL,NULL);
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
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::iterator bintree<Key,T,Cmp>::insert(const pair<Key,T>& p,
			bool replace) {
		return insert(p.first,p.second,replace);
	}

	// === find ===
	template<class Key,class T,class Cmp>
	typename bintree<Key,T,Cmp>::iterator bintree<Key,T,Cmp>::find(const Key& k) {
		bintree_node<Key,T,Cmp>* node = _head.right();
		while(node != NULL) {
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
	template<class Key,class T,class Cmp>
	typename bintree<Key,T,Cmp>::const_iterator bintree<Key,T,Cmp>::find(const Key& k) const {
		iterator it = find(k);
		return const_iterator(it.node());
	}

	// === bounds ===
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::iterator bintree<Key,T,Cmp>::lower_bound(const key_type &x) {
		bintree_node<Key,T,Cmp>* node = _head.right();
		while(node != NULL) {
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
	template<class Key,class T,class Cmp>
	inline typename bintree<Key,T,Cmp>::const_iterator bintree<Key,T,Cmp>::lower_bound(
			const key_type &x) const {
		iterator it = lower_bound(x);
		return const_iterator(it.node());
	}
	template<class Key,class T,class Cmp>
	typename bintree<Key,T,Cmp>::iterator bintree<Key,T,Cmp>::upper_bound(const key_type &x) {
		bintree_node<Key,T,Cmp>* node = _head.right();
		while(node != NULL) {
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
	template<class Key,class T,class Cmp>
	typename bintree<Key,T,Cmp>::const_iterator bintree<Key,T,Cmp>::upper_bound(
			const key_type &x) const {
		iterator it = upper_bound(x);
		return const_iterator(it.node());
	}

	// === erase ===
	template<class Key,class T,class Cmp>
	bool bintree<Key,T,Cmp>::erase(const Key& k) {
		// don't even try it when _elCount is 0
		if(_elCount > 0)
			return do_erase(_head.right(),k);
		return false;
	}
	template<class Key,class T,class Cmp>
	void bintree<Key,T,Cmp>::erase(iterator it) {
		bintree_node<Key,T,Cmp>* node = it.node();
		do_erase(node);
	}
	template<class Key,class T,class Cmp>
	void bintree<Key,T,Cmp>::erase(iterator first,iterator last) {
		while(first != last) {
			bintree_node<Key,T,Cmp>* node = (first++).node();
			do_erase(node);
		}
	}
	template<class Key,class T,class Cmp>
	void bintree<Key,T,Cmp>::clear() {
		bintree_node<Key,T,Cmp>* node = _head.next();
		while(node != &_foot) {
			bintree_node<Key,T,Cmp>* next = node->next();
			delete node;
			node = next;
		}
		_elCount = 0;
		_head.right(NULL);
		_head.next(&_foot);
		_foot.prev(&_head);
	}

	// === helper ===
	template<class Key,class T,class Cmp>
	bintree_node<Key,T,Cmp>* bintree<Key,T,Cmp>::find_min(bintree_node<Key,T,Cmp>* n) {
		bintree_node<Key,T,Cmp>* current = n;
		while(current->left())
			current = current->left();
		return current;
	}
	template<class Key,class T,class Cmp>
	void bintree<Key,T,Cmp>::replace_in_parent(bintree_node<Key,T,Cmp>* n,
			bintree_node<Key,T,Cmp>* newnode) {
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
	template<class Key,class T,class Cmp>
	bool bintree<Key,T,Cmp>::do_erase(bintree_node<Key,T,Cmp>* n,const Key& k) {
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
	template<class Key,class T,class Cmp>
	void bintree<Key,T,Cmp>::do_erase(bintree_node<Key,T,Cmp>* n) {
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
			replace_in_parent(n,NULL);
		_elCount--;
	}

	// === global operators ===
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
