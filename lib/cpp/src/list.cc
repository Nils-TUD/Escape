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

namespace esc {
	// === listnode ===
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
			: _prev(NULL), _next(NULL), _data(T()) {
		};
		inline listnode(listnode<T> *prev,listnode<T> *next)
			: _prev(prev), _next(next), _data(T()) {
		};
		inline listnode(listnode<T> *prev,listnode<T> *next,T data)
			: _prev(prev), _next(next), _data(data) {
		};
		inline ~listnode() {
		};

		inline listnode(const listnode<T>& l) : _prev(l.prev()), _next(l.next()), _data(l.data()) {
		};
		listnode<T>& operator=(const listnode<T>& l) {
			_prev = l.prev();
			_next = l.next();
			_data = l.data();
			return *this;
		};

		inline listnode<T>* prev() const {
			return _prev;
		};
		inline void prev(listnode<T>* p) {
			_prev = p;
		};
		inline listnode<T>* next() const {
			return _next;
		};
		inline void next(listnode<T>* n) {
			_next = n;
		};
		inline T data() const {
			return _data;
		};
		inline void data(T d) {
			_data = d;
		};

	private:
		listnode<T>* _prev;
		listnode<T>* _next;
		T _data;
	};

	// === listiterator ===
	template<class T>
	class listiterator : public iterator<bidirectional_iterator_tag,T> {
		friend class list<T>;

	public:
		inline listiterator()
			: _node(NULL) {
		};
		inline listiterator(listnode<T> *n)
			: _node(n) {
		};
		inline ~listiterator() {
		};

		inline T& operator *() const {
			return _node->_data;
		};
		inline T* operator ->() const {
			return &(operator*());
		};
		inline listiterator operator +(typename listiterator<T>::difference_type n) const {
			listnode<T> *node = _node;
			while(n-- > 0)
				node = node->next();
			return listiterator<T>(node);
		};
		listiterator& operator +=(typename listiterator<T>::difference_type n) {
			while(n-- > 0)
				_node = _node->next();
			return *this;
		};
		listiterator operator -(typename listiterator<T>::difference_type n) const {
			listnode<T> *node = _node;
			while(n-- > 0)
				node = node->prev();
			return listiterator<T>(node);
		};
		listiterator& operator -=(typename listiterator<T>::difference_type n) {
			while(n-- > 0)
				_node = _node->prev();
			return *this;
		};
		inline listiterator& operator ++() {
			_node = _node->next();
			return *this;
		};
		inline listiterator operator ++(int) {
			listiterator<T> tmp(*this);
			operator++();
			return tmp;
		};
		inline listiterator& operator --() {
			_node = _node->prev();
			return *this;
		};
		inline listiterator operator --(int) {
			listiterator<T> tmp(*this);
			operator--();
			return tmp;
		};
		inline bool operator ==(const listiterator<T>& rhs) {
			return _node == rhs._node;
		};
		inline bool operator !=(const listiterator<T>& rhs) {
			return _node != rhs._node;
		};

	private:
		inline listnode<T>* node() const {
			return _node;
		};

	private:
		listnode<T>* _node;
	};

	// === const_listiterator ===
	template<class T>
	class const_listiterator : public iterator<bidirectional_iterator_tag,T> {
		friend class list<T>;

	public:
		inline const_listiterator()
			: _node(NULL) {
		};
		inline const_listiterator(const listnode<T> *n)
			: _node(n) {
		};
		inline ~const_listiterator() {
		};

		inline const T& operator *() const {
			return _node->_data;
		};
		inline const T* operator ->() const {
			return &(operator*());
		};
		const_listiterator operator +(typename const_listiterator<T>::difference_type n) const {
			listnode<T> *node = _node;
			while(n-- > 0)
				node = node->next();
			return const_listiterator<T>(node);
		};
		const_listiterator& operator +=(typename const_listiterator<T>::difference_type n) {
			while(n-- > 0)
				_node = _node->next();
			return *this;
		};
		const_listiterator operator -(typename const_listiterator<T>::difference_type n) const {
			listnode<T> *node = _node;
			while(n-- > 0)
				node = node->prev();
			return const_listiterator<T>(node);
		};
		const_listiterator& operator -=(typename const_listiterator<T>::difference_type n) {
			while(n-- > 0)
				_node = _node->prev();
			return *this;
		};
		inline const_listiterator& operator ++() {
			_node = _node->next();
			return *this;
		};
		inline const_listiterator operator ++(int) {
			const_listiterator<T> tmp(*this);
			operator++();
			return tmp;
		};
		inline const_listiterator& operator --() {
			_node = _node->prev();
			return *this;
		};
		inline const_listiterator operator --(int) {
			const_listiterator<T> tmp(*this);
			operator--();
			return tmp;
		};
		inline bool operator ==(const const_listiterator<T>& rhs) {
			return _node == rhs._node;
		};
		inline bool operator !=(const const_listiterator<T>& rhs) {
			return _node != rhs._node;
		};

	private:
		inline const listnode<T>* node() const {
			return _node;
		};

	private:
		const listnode<T>* _node;
	};

	// === constructors/destructors ===
	template<class T>
	list<T>::list()
		: _count(0), _head(listnode<T>(NULL,NULL,T())), _foot(NULL,NULL,T()) {
		_head.next(&_foot);
		_foot.prev(&_head);
	}
	template<class T>
	list<T>::list(size_type n,const T& value)
		: _count(0), _head(listnode<T>(NULL,NULL,T())), _foot(NULL,NULL,T()) {
		_head.next(&_foot);
		_foot.prev(&_head);
		insert(begin(),n,value);
	}
	template<class T>
	template<class InputIterator>
	list<T>::list(InputIterator first,InputIterator last)
		: _count(0), _head(listnode<T>(NULL,NULL,T())), _foot(NULL,NULL,T()) {
		_head.next(&_foot);
		_foot.prev(&_head);
		insert(begin(),first,last);
	}
	template<class T>
	list<T>::list(const list<T>& x)
		: _count(0), _head(listnode<T>(NULL,NULL,T())), _foot(NULL,NULL,T()) {
		_head.next(&_foot);
		_foot.prev(&_head);
		insert(begin(),x.begin(),x.end());
	}
	template<class T>
	inline list<T>::~list() {
		clear();
	}

	// === assigns ===
	template<class T>
	list<T>& list<T>::operator =(const list<T>& x) {
		clear();
		insert(begin(),x.begin(),x.end());
		return *this;
	}
	template<class T>
	template<class InputIterator>
	void list<T>::assign(InputIterator first,InputIterator last) {
		clear();
		insert(begin(),first,last);
	}
	template<class T>
	void list<T>::assign(size_type n,const T& t) {
		clear();
		insert(begin(),n,t);
	}

	// === iterator stuff ===
	template<class T>
	inline typename list<T>::iterator list<T>::begin() {
		return iterator(_head.next());
	}
	template<class T>
	inline typename list<T>::const_iterator list<T>::begin() const {
		return const_iterator(_head.next());
	}
	template<class T>
	inline typename list<T>::iterator list<T>::end() {
		return iterator(&_foot);
	}
	template<class T>
	inline typename list<T>::const_iterator list<T>::end() const {
		return const_iterator(&_foot);
	}
	template<class T>
	inline typename list<T>::reverse_iterator list<T>::rbegin() {
		return reverse_iterator(&_foot);
	}
	template<class T>
	inline typename list<T>::const_reverse_iterator list<T>::rbegin() const {
		return const_reverse_iterator(&_foot);
	}
	template<class T>
	inline typename list<T>::reverse_iterator list<T>::rend() {
		return reverse_iterator(_head.next());
	}
	template<class T>
	inline typename list<T>::const_reverse_iterator list<T>::rend() const {
		return const_reverse_iterator(_head.next());
	}

	// === size stuff ===
	template<class T>
	inline bool list<T>::empty() const {
		return _count == 0;
	}
	template<class T>
	inline typename list<T>::size_type list<T>::size() const {
		return _count;
	}
	template<class T>
	inline typename list<T>::size_type list<T>::max_size() const {
		// TODO whats a usefull value here??
		return ULONG_MAX / sizeof(T);
	}
	template<class T>
	void list<T>::resize(size_type sz,T c) {
		while(_count < sz)
			push_back(c);
		while(_count > sz)
			pop_back();
	}

	// === front/back ===
	template<class T>
	inline typename list<T>::reference list<T>::front() {
		return *begin();
	}
	template<class T>
	inline typename list<T>::const_reference list<T>::front() const {
		return *begin();
	}
	template<class T>
	inline typename list<T>::reference list<T>::back() {
		return *--end();
	}
	template<class T>
	inline typename list<T>::const_reference list<T>::back() const {
		return *--end();
	}

	// === push/pop ===
	template<class T>
	inline void list<T>::push_front(const T& x) {
		insert(begin(),x);
	}
	template<class T>
	inline void list<T>::pop_front() {
		erase(begin());
	}
	template<class T>
	inline void list<T>::push_back(const T& x) {
		insert(end(),x);
	}
	template<class T>
	inline void list<T>::pop_back() {
		erase(--end());
	}

	// === insert ===
	template<class T>
	typename list<T>::iterator list<T>::insert(iterator position,const T& x) {
		listnode<T>* node = position.node();
		listnode<T>* prev = new listnode<T>(node->prev(),node,x);
		node->prev()->next(prev);
		node->prev(prev);
		_count++;
		return prev;
	}
	template<class T>
	void list<T>::insert(iterator position,size_type n,const T& x) {
		for(size_type i = 0; i < n; i++) {
			position = insert(position,x);
			position++;
		}
	}
	template<class T>
	template<class InputIterator>
	void list<T>::insert(iterator position,InputIterator first,InputIterator last) {
		while(first != last) {
			position = insert(position,*first++);
			position++;
		}
	}

	// === erase ===
	template<class T>
	typename list<T>::iterator list<T>::erase(iterator position) {
		listnode<T>* node = position.node();
		listnode<T>* next = node->next(), *prev = node->prev();
		prev->next(next);
		next->prev(prev);
		delete node;
		_count--;
		return next;
	}
	template<class T>
	typename list<T>::iterator list<T>::erase(iterator position,iterator last) {
		while(position != last)
			position = erase(position);
		return position;
	}
	template<class T>
	void list<T>::clear() {
		// free mem
		listnode<T>* node = _head.next();
		while(node != &_foot) {
			listnode<T> *tmp = node;
			node = node->next();
			delete tmp;
		}
		// reset
		_count = 0;
		_head.next(&_foot);
		_foot.prev(&_head);
	}

	// === splice ===
	template<class T>
	inline void list<T>::splice(iterator position,list<T>& x) {
		splice(position,x,x.begin(),x.end());
	}
	template<class T>
	inline void list<T>::splice(iterator position,list<T>& x,iterator i) {
		iterator end = i;
		splice(position,x,i,++end);
	}
	template<class T>
	void list<T>::splice(iterator position,list<T>& x,iterator first,iterator last) {
		if(&x == this) {
			listnode<T>* pnode = position.node();
			listnode<T>* fnode = first.node();
			listnode<T>* lnode = last.node()->prev();
			// remove
			fnode->prev()->next(lnode->next());
			lnode->next()->prev(fnode->prev());
			// insert
			pnode->prev()->next(fnode);
			fnode->prev(pnode->prev());
			pnode->prev(lnode);
			lnode->next(pnode);
		}
		else {
			while(first != last) {
				position = insert(position,*first);
				first = x.erase(first);
				position++;
			}
		}
	}

	// === remove ===
	template<class T>
	void list<T>::remove(const T& value) {
		for(iterator it = begin(); it != end(); ) {
			if(*it == value)
				it = erase(it);
			else
				it++;
		}
	}
	template<class T>
	template<class Predicate>
	void list<T>::remove_if(Predicate pred) {
		for(iterator it = begin(); it != end(); ) {
			if(pred(*it))
				it = erase(it);
			else
				it++;
		}
	}

	// === unique ===
	template<class T>
	void list<T>::unique() {
		iterator it = begin();
		for(it++; it != end(); ) {
			if(*it == *(it - 1))
				it = erase(it);
			else
				it++;
		}
	}
	template<class T>
	template<class BinaryPredicate>
	void list<T>::unique(BinaryPredicate binary_pred) {
		iterator it = begin();
		for(it++; it != end(); ) {
			if(binary_pred(*it,*(it - 1)))
				it = erase(it);
			else
				it++;
		}
	}

	template<class T>
	inline static T defaultCompare(T a,T b) {
		return a - b;
	}

	// === merge ===
	template<class T>
	inline void list<T>::merge(list<T>& x) {
		merge(x,defaultCompare<T>);
	}
	template<class T>
	template<class Compare>
	void list<T>::merge(list<T>& x,Compare comp) {
		for(iterator xit = x.begin(); xit != x.end(); ) {
			iterator it = begin();
			for(; it != end(); it++) {
				if(comp(*xit,*it) < 0)
					break;
			}
			insert(it,*xit);
			xit = x.erase(xit);
		}
	}

	// === sort ===
	template<class T>
	inline void list<T>::sort() {
		esc::sort(begin(),end());
	}
	template<class T>
	template<class Compare>
	inline void list<T>::sort(Compare comp) {
		esc::sort(begin(),end(),comp);
	}

	// === reverse and swap ===
	template<class T>
	inline void list<T>::reverse() {
		esc::reverse(begin(),end());
	}
	template<class T>
	inline void list<T>::swap(list<T>& x) {
		esc::swap<list<T> >(*this,x);
	}
}
