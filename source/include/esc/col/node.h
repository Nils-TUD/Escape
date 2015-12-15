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

#include <esc/col/internal.h>
#include <sys/common.h>

namespace esc {

template<class T>
class SList;
template<class T>
class DList;
template<
	template<class T1>
	class LIST,
	class T
>
class IList;
template<class T,class It>
class ListIteratorBase;
class NodeAllocator;
class DListItem;

/**
 * A listitem for the singly linked list. It is intended that you inherit from this class to add
 * data to the item.
 */
class SListItem : public CacheAllocatable {
	template<class T>
	friend class SList;
	template<class T,class It>
	friend class ListIteratorBase;
	friend class NodeAllocator;
	friend class DListItem;

public:
	/**
	 * Constructor
	 */
	explicit SListItem() : _next() {
	}

	void init() {
		_next = nullptr;
	}

private:
	SListItem *next() {
		return _next;
	}
	void next(SListItem *i) {
		_next = i;
	}

	SListItem *_next;
};

/**
 * A listitem for the doubly linked list. It is intended that you inherit from this class to add
 * data to the item.
 */
class DListItem : public SListItem {
	template<class T>
	friend class SList;
	template<class T>
	friend class DList;
	template<class T, class It>
	friend class ListIteratorBase;
	friend class NodeAllocator;

public:
	/**
	 * Constructor
	 */
	explicit DListItem() : SListItem(), _prev() {
	}

	void init() {
		SListItem::init();
		_prev = nullptr;
	}

private:
	DListItem *next() {
		return static_cast<DListItem*>(SListItem::next());
	}
	void next(DListItem *i) {
		SListItem::next(i);
	}
	DListItem *prev() {
		return _prev;
	}
	void prev(DListItem *i) {
		_prev = i;
	}

	DListItem *_prev;
};

/**
 * The node for indirect linked list.
 */
template<class T>
struct IListNode : public DListItem {
	explicit IListNode(T _data) : DListItem(), data(_data) {
		static_assert(sizeof(T) <= sizeof(void*),"T has to be a word!");
	}

	static void *operator new(size_t) throw() {
		return NodeAllocator::allocate();
	}

	static void operator delete(void *ptr) throw() {
		NodeAllocator::free(ptr);
	}

	T data;
};

/**
 * Generic iterator for a linked list. Expects the list node class to have a next() method.
 */
template<class T,class It>
class ListIteratorBase {
	template<class T1>
	friend class SList;
	template<
		template<class T1>
		class LIST,
		class T2
	>
	friend class IList;

public:
	explicit ListIteratorBase(T *n = nullptr) : _n(n) {
	}

	It& operator++() {
		_n = static_cast<T*>(_n->next());
		return static_cast<It&>(*this);
	}
	It operator++(int) {
		It tmp(static_cast<It&>(*this));
		operator++();
		return tmp;
	}
	bool operator==(const It& rhs) const {
		return _n == rhs._n;
	}
	bool operator!=(const It& rhs) const {
		return _n != rhs._n;
	}

protected:
	T *_n;
};

/**
 * Default list iterator
 */
template<class T>
class ListIterator : public ListIteratorBase<T,ListIterator<T>> {
public:
	explicit ListIterator(T *n = nullptr)
			: ListIteratorBase<T,ListIterator<T>>(n) {
	}

	T & operator*() const {
		return *this->_n;
	}
	T *operator->() const {
		return &operator*();
	}
};

/**
 * Default const list iterator
 */
template<class T>
class ListConstIterator : public ListIteratorBase<T,ListConstIterator<T>> {
public:
	explicit ListConstIterator(T *n = nullptr)
			: ListIteratorBase<T,ListConstIterator<T>>(n) {
	}

	const T & operator*() const {
		return *this->_n;
	}
	const T *operator->() const {
		return &operator*();
	}
};

/**
 * Indirect list iterator
 */
template<class T>
class IListIterator : public ListIteratorBase<IListNode<T>,IListIterator<T>> {
	typedef ListIteratorBase<IListNode<T>,IListIterator<T>> base_type;
public:
	explicit IListIterator(IListNode<T> *n = nullptr)
			: base_type(n) {
	}

	T & operator*() const {
		return this->_n->data;
	}
	T *operator->() const {
		return &operator*();
	}
};

/**
 * Indirect const list iterator
 */
template<class T>
class IListConstIterator : public ListIteratorBase<IListNode<T>,IListConstIterator<T>> {
	typedef ListIteratorBase<IListNode<T>,IListConstIterator<T>> base_type;
public:
	explicit IListConstIterator(const IListNode<T> *n = nullptr)
			: base_type(const_cast<IListNode<T>*>(n)) {
	}

	const T & operator*() const {
		return this->_n->data;
	}
	const T *operator->() const {
		return &operator*();
	}
};

}
