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

#include <sys/common.h>
#include <sys/col/islist.h>
#include <sys/mem/pagedir.h>

DynArray NodeAllocator::nodeArray(sizeof(IListNode<void*>),SLLNODE_AREA,SLLNODE_AREA_SIZE);
SListItem *NodeAllocator::freelist = NULL;
klock_t NodeAllocator::lock;

void *NodeAllocator::allocate() {
	SpinLock::acquire(&lock);
	if(freelist == NULL) {
		size_t oldCount = nodeArray.getObjCount();
		if(!nodeArray.extend()) {
			SpinLock::release(&lock);
			return NULL;
		}
		for(size_t i = oldCount; i < nodeArray.getObjCount(); i++) {
			SListItem *n = (SListItem*)nodeArray.getObj(i);
			n->next(freelist);
			freelist = n;
		}
	}
	SListItem *n = freelist;
	freelist = freelist->next();
	SpinLock::release(&lock);
	return n;
}

void NodeAllocator::free(void *ptr) {
	SListItem *n = (SListItem*)ptr;
	SpinLock::acquire(&lock);
	n->next(freelist);
	freelist = n;
	SpinLock::release(&lock);
}
