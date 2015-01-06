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
#include <esc/col/treap.h>
#include <esc/col/dlist.h>

namespace esc {

/**
 * A node in the slist-treap. You may create a subclass of this to add data to your nodes.
 */
template<typename KEY>
class DListTreapNode : public TreapNode<KEY> {
public:
    /**
     * Constructor
     *
     * @param key the key of the node
     */
    explicit DListTreapNode(typename TreapNode<KEY>::key_t key)
        : TreapNode<KEY>(key), _prev(nullptr), _next(nullptr) {
    }

    DListTreapNode *prev() {
        return _prev;
    }
    void prev(DListTreapNode *i) {
        _prev = i;
    }
    DListTreapNode *next() {
        return _next;
    }
    void next(DListTreapNode *i) {
        _next = i;
    }

private:
    DListTreapNode *_prev;
    DListTreapNode *_next;
};

/**
 * A combination of a doubly linked list and a treap, so that you can both iterate over all items
 * and find items by a key quickly. Note that the list does not maintain the order of the keys,
 * but has an arbitrary order.
 */
template<class T>
class DListTreap {
public:
    typedef typename DList<T>::iterator iterator;
    typedef typename DList<T>::const_iterator const_iterator;

    /**
     * Creates an empty slist-treap
     */
    explicit DListTreap() : _list(), _tree() {
    }

    /**
     * @return the number of items in the list
     */
    size_t length() const {
        return _list.length();
    }
    /**
     * Clears the dlist-treap (does not free nodes)
     */
    void clear() {
        _list.clear();
        _tree.clear();
    }

    /**
     * @return beginning of list (you can change the list items)
     */
    iterator begin() {
        return _list.begin();
    }
    /**
     * @return end of list
     */
    iterator end() {
        return _list.end();
    }
    /**
     * @return tail of the list, i.e. the last valid item
     */
    iterator tail() {
        return _list.tail();
    }

    /**
     * @return beginning of list (you can NOT change the list items)
     */
    const_iterator cbegin() const {
        return _list.cbegin();
    }
    /**
     * @return end of list
     */
    const_iterator cend() const {
        return _list.cend();
    }
    /**
     * @return tail of the list, i.e. the last valid item (NOT changeable)
     */
    const_iterator ctail() const {
        return _list.ctail();
    }

    /**
     * Finds the node with given key in the tree
     *
     * @param key the key
     * @return the node or nullptr if not found
     */
    T *find(typename T::key_t key) const {
        return _tree.find(key);
    }

    /**
     * Inserts the given node in the tree. Note that it is expected, that the key of the node is
     * already set.
     *
     * @param node the node to insert
     */
    void insert(T *node) {
        _list.append(node);
        _tree.insert(node);
    }

    /**
     * Removes the given node from the tree.
     *
     * @param node the node to remove
     */
    void remove(T *node) {
        _list.remove(node);
        _tree.remove(node);
    }

private:
    DList<T> _list;
    Treap<T> _tree;
};

}
