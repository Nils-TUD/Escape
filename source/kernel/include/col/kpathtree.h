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

#include <common.h>
#include <esc/pathtree.h>
#include <cppsupport.h>
#include <ostream.h>
#include <log.h>

template<class T>
class KPathTree;

template<class T>
class KPathTreeItem : public esc::PathTreeItem<T>, public CacheAllocatable {
	template<class T2>
	friend class KPathTree;

public:
	explicit KPathTreeItem(char *name,T *data = NULL)
		: esc::PathTreeItem<T>(name,data), CacheAllocatable() {
	}
};

/**
 * A path tree is like a prefix-tree for path-items. It creates a hierarchy of path-items and
 * stores data for items that have been inserted explicitly. Intermediate-items have NULL as
 * data. You can insert paths into the tree, find them and remove them again.
 * All paths can contain multiple slashes in a row, ".." and ".".
 */
template<class T>
class KPathTree : public esc::PathTree<T,KPathTreeItem<T>> {
public:
	typedef void (*fPrintItem)(OStream &os,T *data);

	/**
	 * Prints the tree to the given stream.
	 *
	 * @param os the stream
	 * @param printItem a function that is called for every item for printing it data-specific
	 */
	void print(OStream &os,fPrintItem printItem = NULL) const {
		printRec(os,this->_root,0,printItem);
	}

private:
	void printRec(OStream &os,KPathTreeItem<T> *item,int layer,fPrintItem printItem) const {
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
			KPathTreeItem<T> *n = static_cast<KPathTreeItem<T>*>(item->_child);
			while(n) {
				printRec(os,n,layer + 1,printItem);
				n = static_cast<KPathTreeItem<T>*>(n->_next);
			}
		}
	}
};
