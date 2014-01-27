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

#include <sys/common.h>
#include <sys/col/node.h>
#include <assert.h>

/**
 * The singly linked list. Takes an arbitrary class as list-item and expects it to have a prev(),
 * prev(T*), next() and next(*T) method. In most cases, you should inherit from SListItem and
 * specify your class for T.
 */
template<class T>
class SList : public CacheAllocatable {
public:
	typedef ListIterator<T> iterator;
	typedef ListConstIterator<T> const_iterator;

	/**
	 * Constructor. Creates an empty list
	 */
	explicit SList()
			: _head(nullptr), _tail(nullptr), _len(0) {
	}

	/**
	 * @return the number of items in the list
	 */
	size_t length() const {
		return _len;
	}

	/**
	 * @return beginning of list (you can change the list items)
	 */
	iterator begin() {
		return iterator(_head);
	}
	/**
	 * @return end of list
	 */
	iterator end() {
		return iterator();
	}
	/**
	 * @return tail of the list, i.e. the last valid item
	 */
	iterator tail() {
		return iterator(_tail);
	}

	/**
	 * @return beginning of list (you can NOT change the list items)
	 */
	const_iterator cbegin() const {
		return const_iterator(_head);
	}
	/**
	 * @return end of list
	 */
	const_iterator cend() const {
		return const_iterator();
	}
	/**
	 * @return tail of the list, i.e. the last valid item (NOT changeable)
	 */
	const_iterator ctail() const {
		return const_iterator(_tail);
	}

	/**
	 * Appends the given item to the list. This works in constant time.
	 *
	 * @param e the list item
	 * @return the position where it has been inserted
	 */
	iterator append(T *e) {
		if(_head == nullptr)
			_head = e;
		else
			_tail->next(e);
		_tail = e;
		e->next(nullptr);
		_len++;
		return iterator(e);
	}

	/**
	 * Inserts the given item into the list after <p>. This works in constant time.
	 *
	 * @param p the previous item (p = insert it at the beginning)
	 * @param e the list item
	 * @return the position where it has been inserted
	 */
	iterator insert(T *p,T *e) {
		e->next(p ? p->next() : _head);
		if(p)
			p->next(e);
		else
			_head = e;
		if(!e->next())
			_tail = e;
		_len++;
		return iterator(e);
	}

	/**
	 * Removes the given item from the list. This works in linear time.
	 * Does NOT expect that the item is in the list!
	 *
	 * @param e the list item (doesn't have to be a valid pointer)
	 * @return true if the item has been found and removed
	 */
	bool remove(T *e) {
		T *t = _head,*p = nullptr;
		while(t && t != e) {
			p = t;
			t = static_cast<T*>(t->next());
		}
		if(!t)
			return false;
		removeAt(p,e);
		return true;
	}

	/**
	 * Removes the given item from the list and expects that <p> is the previous item. This works
	 * in constant time.
	 *
	 * @param p the prev of e
	 * @param e the element to remove
	 */
	void removeAt(T *p,T *e) {
		if(p)
			p->next(e->next());
		else
			_head = static_cast<T*>(e->next());
		if(!e->next())
			_tail = p;
		_len--;
	}

	/**
	 * Removes the first element from the list and returns it.
	 *
	 * @return the first element or NULL if the list is empty
	 */
	T *removeFirst() {
		if(!_len)
			return nullptr;
		T *res = &*begin();
		removeAt(nullptr,res);
		return res;
	}

	/**
	 * Clears the list. Note that it leaves the elements untouched, i.e. the next-pointer isn't
	 * changed.
	 */
	void clear() {
		_head = _tail = nullptr;
		_len = 0;
	}

	/**
	 * Deletes all elements in this list and clears the list.
	 */
	void deleteAll() {
		for(auto it = begin(); it != end(); ) {
			auto old = it++;
			delete &*old;
		}
		clear();
	}

private:
	T *_head;
	T *_tail;
	size_t _len;
};
