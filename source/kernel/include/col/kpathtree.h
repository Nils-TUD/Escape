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

/**
 * The in-kernel variant of the PathTree, which adds printing capabilities.
 */
template<class T,class ITEM>
class KPathTree : public esc::PathTree<T,ITEM> {
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
	void printRec(OStream &os,ITEM *item,int layer,fPrintItem printItem) const {
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
			ITEM *n = static_cast<ITEM*>(item->_child);
			while(n) {
				printRec(os,n,layer + 1,printItem);
				n = static_cast<ITEM*>(n->_next);
			}
		}
	}
};
