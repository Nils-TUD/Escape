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

#pragma once

#include <sys/common.h>
#include <sys/col/slist.h>

/**
 * An indirect, singly linked list. That is, the elements are not inherited from a class that gives
 * us a next-pointer, but we have nodes that form the list and the nodes contain a pointer to the
 * element. This allows us a add an element to multiple lists.
 */
template<class T>
class ISList {
	struct Node : public SListItem {
		explicit Node(T _data) : SListItem(), data(_data) {
		}
		T data;
	};

	class ISListIterator : public SListIteratorBase<Node, ISListIterator> {
	public:
	    explicit ISListIterator(Node *n = nullptr)
	    	: SListIteratorBase<Node, ISListIterator>(n) {
	    }

	    T & operator*() const {
	        return this->_n->data;
	    }
	    T *operator->() const {
	        return &operator*();
	    }
	};

	class ISListConstIterator : public SListIteratorBase<Node, ISListConstIterator> {
	public:
	    explicit ISListConstIterator(const Node *n = nullptr)
	    	: SListIteratorBase<Node, ISListConstIterator>(const_cast<Node*>(n)) {
	    }

	    const T & operator*() const {
	        return this->_n->data;
	    }
	    const T *operator->() const {
	        return &operator*();
	    }
	};

public:
    typedef ISListIterator iterator;
    typedef ISListConstIterator const_iterator;

    /**
     * Constructor. Creates an empty list
     */
    explicit ISList() : list() {
    }
    /**
     * Destructor. Free's all memory.
     */
    ~ISList() {
    	clear();
    }

    /**
     * @return the number of items in the list
     */
    size_t length() const {
        return list.length();
    }

    /**
     * @return beginning of list (you can change the list items)
     */
    iterator begin() {
        return iterator(&*list.begin());
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
        return iterator(&*list.tail());
    }

    /**
     * @return beginning of list (you can NOT change the list items)
     */
    const_iterator cbegin() const {
        return const_iterator(&*list.cbegin());
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
        return const_iterator(&*list.ctail());
    }

    /**
     * Appends the given item to the list. This works in constant time.
     *
     * @param e the list item
     * @return true if successfull
     */
    bool append(T e) {
    	/* TODO we should use a sllnodes-like allocator here! */
    	Node *n = new Node(e);
    	if(n)
    		list.append(n);
    	return n != nullptr;
    }

    /**
     * Removes the given item from the list. This works in linear time.
     * Does NOT expect that the item is in the list!
     *
     * @param e the list item (doesn't have to be a valid pointer)
     * @return true if the item has been found and removed
     */
    bool remove(T e) {
    	Node *p = nullptr;
    	for(iterator it = begin(); it != end(); p = it._n, ++it) {
    		if(*it == e) {
    			list.removeAt(p,it._n);
    			delete it._n;
    			return true;
    		}
    	}
    	return false;
    }

    /**
     * Clears the list, i.e. deletes all nodes.
     */
    void clear() {
    	for(auto it = begin(); it != end(); ) {
    		auto old = it++;
    		delete old._n;
    	}
    	list.clear();
    }

private:
    SList<Node> list;
};
