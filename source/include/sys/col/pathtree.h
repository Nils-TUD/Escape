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
#include <sys/cppsupport.h>
#include <sys/ostream.h>
#include <sys/log.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

template<class T>
class PathTree;

/**
 * An item in a path-tree with an arbitrary pointer as data.
 */
template<class T>
class PathTreeItem : public CacheAllocatable {
	template<class T2>
	friend class PathTree;

public:
	explicit PathTreeItem(char *name,T *data = NULL)
		: _name(name), _namelen(strlen(name)), _data(data), _next(), _parent(), _child() {
	}
	PathTreeItem(const PathTreeItem<T> &i)
		: _name(strdup(i._name)), _namelen(i._namelen), _data(i._data), _next(), _parent(), _child() {
	}
	~PathTreeItem() {
		cache_free(_name);
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
template<class T>
class PathTree {
public:
	typedef void (*fPrintItem)(OStream &os,T *data);

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
	 * Replaces the whole tree with <src>, i.e. clones all nodes
	 *
	 * @param src the tree to clone
	 * @return 0 on success
	 */
	int replaceWith(const PathTree<T> &src) {
		PathTreeItem<T> *old = _root;
		_root = cloneRec(src._root);
		if(src._root && !_root)
			return -ENOMEM;
		destroyRec(old);
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
		PathTreeItem<T> *last;
		find(pathend,last);
		/* is root missing? */
		if(!last) {
			PathTreeItem<T> *i = createItem("/",1);
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
			PathTreeItem<T> *i = findAt(last,pathend,len);
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
	PathTreeItem<T> *find(const char *path,const char **end = NULL) {
		PathTreeItem<T> *last;
		PathTreeItem<T> *match = find(path,last);
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
		PathTreeItem<T> *last;
		PathTreeItem<T> *match = find(path,last);
		if(last && match == last && !*path) {
			res = match->getData();
			last->_data = NULL;
			/* delete now unnecessary nodes (that don't have data and childs) */
			while(last->_data == NULL && last->_child == NULL) {
				/* remove from parent */
				if(last->_parent != last) {
					PathTreeItem<T> *n = last->_parent->_child;
					/* find prev */
					PathTreeItem<T> *p = NULL;
					while(n) {
						if(n == last)
							break;
						p = n;
						n = n->_next;
					}
					assert(n == last);
					/* remove from list */
					if(p)
						p->_next = n->_next;
					else
						last->_parent->_child = n->_next;
				}
				else
					_root = NULL;
				/* delete and go to next */
				PathTreeItem<T> *next = last->_parent;
				delete last;
				/* stop at root */
				if(next == last)
					break;
				last = next;
			}
		}
		return res;
	}

	/**
	 * Prints the tree to the given stream.
	 *
	 * @param os the stream
	 * @param printItem a function that is called for every item for printing it data-specific
	 */
	void print(OStream &os,fPrintItem printItem = NULL) const {
		printRec(os,_root,0,printItem);
	}

private:
	PathTreeItem<T> *find(const char *&path,PathTreeItem<T> *&last) {
		PathTreeItem<T> *n = _root;
		PathTreeItem<T> *match = n && n->_data ? n : NULL;
		last = NULL;
		while(n) {
			last = n;
			size_t len = toNext(path,n);
			if(!len)
				break;

			last = n;
			n = findAt(n,path,len);
			if(n) {
				if(n->_data)
					match = n;
				path += len;
			}
		}
		return match;
	}

	PathTreeItem<T> *cloneRec(PathTreeItem<T> *src) {
		PathTreeItem<T> *clone = NULL;
		if(src) {
			clone = new PathTreeItem<T>(*src);
			/* strdup might have failed */
			if(!clone->_name) {
				delete clone;
				return NULL;
			}
			/* now clone childs */
			PathTreeItem<T> *n = src->_child;
			while(n) {
				PathTreeItem<T> *nn = cloneRec(n);
				if(!nn) {
					destroyRec(clone);
					return NULL;
				}
				nn->_next = clone->_child;
				clone->_child = nn;
				nn->_parent = clone;
				n = n->_next;
			}
		}
		return clone;
	}

	void destroyRec(PathTreeItem<T> *item) {
		if(item) {
			PathTreeItem<T> *n = item->_child;
			while(n) {
				PathTreeItem<T> *next = n->_next;
				destroyRec(n);
				n = next;
			}
			delete item;
		}
	}

	void printRec(OStream &os,PathTreeItem<T> *item,int layer,fPrintItem printItem) const {
		if(item) {
			os.writef("%*s%-*s",layer,"",12 - layer,item->_name);
			if(item->_data) {
				os.writef(" -> ");
				if(printItem)
					printItem(os,item->_data);
				else
					os.writef("%p",item->_data);
			}
			os.writef("\n");
			PathTreeItem<T> *n = item->_child;
			while(n) {
				printRec(os,n,layer + 1,printItem);
				n = n->_next;
			}
		}
	}

	static PathTreeItem<T> *findAt(PathTreeItem<T> *n,const char *path,size_t len) {
		n = n->_child;
		while(n) {
			if(n->_namelen == len && strncmp(n->_name,path,len) == 0)
				return n;
			n = n->_next;
		}
		return NULL;
	}

	static PathTreeItem<T> *createItem(const char *src,size_t len) {
		char *name = (char*)cache_alloc(len + 1);
		if(!name)
			return NULL;
		strnzcpy(name,src,len + 1);
		PathTreeItem<T> *i = new PathTreeItem<T>(name);
		if(!i) {
			cache_free(name);
			return NULL;
		}
		return i;
	}

	static size_t toNext(const char *&path,PathTreeItem<T> *&n) {
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
				n = n->_parent;
				continue;
			}
			while(*path == '/')
				path++;
			return pos;
		}
		return 0;
	}

	PathTreeItem<T> *_root;
};
