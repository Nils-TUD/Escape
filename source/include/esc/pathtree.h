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
#include <assert.h>
#include <errno.h>
#include <string.h>

#if defined(IN_KERNEL)
#	include <cwrap.h>
#	define malloc(s)	cache_alloc(s)
#	define free(p)		cache_free(p)
#else
#	include <stdlib.h>
#endif

template<class T,class ITEM>
class KPathTree;

namespace esc {

template<class T,class ITEM>
class PathTree;

/**
 * An item in a path-tree with an arbitrary pointer as data.
 */
template<class T>
class PathTreeItem {
	template<class T2,class ITEM2>
	friend class PathTree;
	template<class T2,class ITEM2>
	friend class ::KPathTree;

public:
	explicit PathTreeItem(char *name,T *data = NULL)
		: _name(name), _namelen(strlen(name)), _data(data), _next(), _parent(), _child() {
	}
	PathTreeItem(const PathTreeItem<T> &i)
		: _name(strdup(i._name)), _namelen(i._namelen), _data(i._data), _next(), _parent(), _child() {
	}
	~PathTreeItem() {
		free(_name);
	}

	/**
	 * @return the parent item (this for the root item)
	 */
	PathTreeItem<T> *getParent() {
		return _parent;
	}

	/**
	 * @return the path-element-name
	 */
	const char *getName() const {
		return _name;
	}
	/**
	 * @return the data
	 */
	T *getData() const {
		return _data;
	}

private:
	char *_name;
	size_t _namelen;
	T *_data;
	PathTreeItem *_next;
	PathTreeItem *_parent;
	PathTreeItem *_child;
};

/**
 * A path tree is like a prefix-tree for path-items. It creates a hierarchy of path-items and
 * stores data for items that have been inserted explicitly. Intermediate-items have NULL as
 * data. You can insert paths into the tree, find them and remove them again.
 * All paths can contain multiple slashes in a row, ".." and ".".
 */
template<class T,class ITEM = PathTreeItem<T>>
class PathTree {
	template<
		class T2,
		class ITEM2 = PathTreeItem<T2>
	>
	class PathTreeIterator {
	public:
		explicit PathTreeIterator(ITEM2 *n = nullptr) : _n(n) {
		}

		ITEM2 & operator*() const {
			return *this->_n;
		}
		ITEM2 *operator->() const {
			return &operator*();
		}

		PathTreeIterator<T2,ITEM2>& operator++() {
			_n = static_cast<ITEM2*>(_n->_next);
			return *this;
		}
		PathTreeIterator<T2,ITEM2> operator++(int) {
			PathTreeIterator<T2,ITEM2> tmp(*this);
			operator++();
			return tmp;
		}
		bool operator==(const PathTreeIterator<T2,ITEM2>& rhs) const {
			return _n == rhs._n;
		}
		bool operator!=(const PathTreeIterator<T2,ITEM2>& rhs) const {
			return _n != rhs._n;
		}

	protected:
		ITEM2 *_n;
	};

public:
	typedef PathTreeIterator<T,ITEM> iterator;

	/**
	 * Constructor creates an empty tree
	 */
	explicit PathTree() : _root() {
	}
	/**
	 * Destructor free's all memory
	 */
	~PathTree() {
		destroyRec(_root);
	}

	/* no assignment or copy (no good opportunity for feedback on failures) */
	PathTree(const PathTree<T>&) = delete;
	PathTree<T> &operator=(const PathTree<T>&) = delete;

	/**
	 * @return an iterator to walk over the childs of given node
	 */
	iterator begin(ITEM *item) {
		return iterator(item->_child);
	}
	iterator end() {
		return iterator();
	}

	/**
	 * Replaces the whole tree with <src>, i.e. clones all nodes
	 *
	 * @param src the tree to clone
	 * @return 0 on success
	 */
	int replaceWith(const PathTree<T,ITEM> &src) {
		ITEM *nroot = cloneRec(src._root);
		if(src._root && !nroot)
			return -ENOMEM;
		destroyRec(_root);
		_root = nroot;
		_root->_parent = _root;
		return 0;
	}

	/**
	 * Inserts a new path into the tree and associates <data> with it. Intermediate notes might have
	 * to be created.
	 *
	 * @param path the path
	 * @param data the data to associate with the path
	 * @return 0 on success
	 */
	int insert(const char *path,T *data) {
		const char *pathend = path;
		ITEM *last;
		find(pathend,last,false);
		/* is root missing? */
		if(!last) {
			ITEM *i = createItem("/",1);
			if(!i)
				return -ENOMEM;
			_root = i;
			/* root should have itself as parent for ".." */
			i->_parent = i;
			last = i;
		}

		/* create the missing path elements */
		while(*pathend) {
			size_t len = toNext(pathend,last);
			if(!len)
				break;

			/* make sure that it doesn't exist already (think of "test/../test") */
			ITEM *i = findAt(last,pathend,len);
			if(!i) {
				i = createItem(pathend,len);
				if(!i) {
					/* we might already have created intermediate nodes. but for simplicity we keep
					 * them, since we're still in a consistent state */
					return -ENOMEM;
				}
				i->_next = last->_child;
				last->_child = i;
				i->_parent = last;
			}

			last = i;
			pathend += len;
		}
		if(last->_data != NULL)
			return -EEXIST;
		last->_data = data;
		return 0;
	}

	/**
	 * Searches for the given path. It returns the last path-item that had associated data and
	 * gives you the remaining path from this point.
	 * So, for example, if / = 0x1, /foo = NULL and /foo/bar = 0x2 and you search for /foo/bar/test,
	 * the matching item will be ("bar",0x2) and the remaining path "test".
	 *
	 * @param path the path to search for
	 * @param end optionally, a pointer to the remaining path from the returned match
	 * @return the last "matching" item, i.e. the last item with data.
	 */
	ITEM *find(const char *path,const char **end = NULL) {
		ITEM *last;
		ITEM *match = find(path,last,true);
		if(end)
			*end = path;
		return match;
	}

	/**
	 * Removes the given path from the tree. It will also remove now unnecessary intermediate nodes.
	 *
	 * @param path the path to remove
	 * @return the data of the removed path or NULL if not found
	 */
	T *remove(const char *path) {
		T *res = NULL;
		ITEM *last;
		ITEM *match = find(path,last,false);
		if(last && match == last && !*path) {
			res = match->getData();
			last->_data = NULL;
			/* delete now unnecessary nodes (that don't have data and childs) */
			while(last->_data == NULL && last->_child == NULL) {
				/* remove from parent */
				if(last->_parent != last) {
					ITEM *n = static_cast<ITEM*>(last->_parent->_child);
					/* find prev */
					ITEM *p = NULL;
					while(n) {
						if(n == last)
							break;
						p = n;
						n = static_cast<ITEM*>(n->_next);
					}
					assert(n == last);
					/* remove from list */
					if(p)
						p->_next = static_cast<ITEM*>(n->_next);
					else
						last->_parent->_child = static_cast<ITEM*>(n->_next);
				}
				else
					_root = NULL;
				/* delete and go to next */
				ITEM *next = static_cast<ITEM*>(last->_parent);
				delete last;
				/* stop at root */
				if(next == last)
					break;
				last = next;
			}
		}
		return res;
	}

protected:
	ITEM *find(const char *&path,ITEM *&last,bool pathToMatch) {
		ITEM *n = _root;
		ITEM *match = n && n->_data ? n : NULL;
		const char *mpath = path;
		last = NULL;
		while(n) {
			last = n;
			size_t len = toNext(path,n);
			if(!len) {
				// in this case, the path is either empty or contains only "." and "..". we don't want
				// to have that in the remaining path, so directory return the match
				if(last == match)
					return match;
				break;
			}

			last = n;
			n = findAt(n,path,len);
			if(n) {
				path += len;
				if(n->_data) {
					match = n;
					mpath = path;
				}
			}
		}
		if(match && pathToMatch) {
			while(*mpath == '/')
				mpath++;
			path = mpath;
		}
		return match;
	}

	ITEM *cloneRec(ITEM *src) {
		ITEM *clone = NULL;
		if(src) {
			clone = new ITEM(*src);
			/* strdup might have failed */
			if(!clone->_name) {
				delete clone;
				return NULL;
			}
			/* now clone childs */
			ITEM *n = static_cast<ITEM*>(src->_child);
			while(n) {
				ITEM *nn = cloneRec(n);
				if(!nn) {
					destroyRec(clone);
					return NULL;
				}
				nn->_next = static_cast<ITEM*>(clone->_child);
				clone->_child = nn;
				nn->_parent = clone;
				n = static_cast<ITEM*>(n->_next);
			}
		}
		return clone;
	}

	void destroyRec(ITEM *item) {
		if(item) {
			ITEM *n = static_cast<ITEM*>(item->_child);
			while(n) {
				ITEM *next = static_cast<ITEM*>(n->_next);
				destroyRec(n);
				n = next;
			}
			delete item;
		}
	}

	static ITEM *findAt(ITEM *n,const char *path,size_t len) {
		n = static_cast<ITEM*>(n->_child);
		while(n) {
			if(n->_namelen == len && strncmp(n->_name,path,len) == 0)
				return n;
			n = static_cast<ITEM*>(n->_next);
		}
		return NULL;
	}

	static ITEM *createItem(const char *src,size_t len) {
		char *name = (char*)malloc(len + 1);
		if(!name)
			return NULL;
		strnzcpy(name,src,len + 1);
		ITEM *i = new ITEM(name);
		if(!i) {
			free(name);
			return NULL;
		}
		return i;
	}

	static size_t toNext(const char *&path,ITEM *&n) {
		while(1) {
			while(*path == '/')
				path++;
			if(!*path)
				break;

			size_t pos = strchri(path,'/');
			assert(pos > 0);
			if(pos == 1 && path[0] == '.') {
				path++;
				continue;
			}
			if(pos == 2 && path[0] == '.' && path[1] == '.') {
				path += 2;
				n = static_cast<ITEM*>(n->_parent);
				continue;
			}
			return pos;
		}
		return 0;
	}

	ITEM *_root;
};

}

#if defined(IN_KERNEL)
#	undef malloc
#	undef free
#endif
