/*
 * Copyright (C) 2013, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <sys/common.h>
#include <sys/col/islist.h>
#include <sys/mem/paging.h>

DynArray NodeAllocator::nodeArray(sizeof(Node<void*>),SLLNODE_AREA,SLLNODE_AREA_SIZE);
SListItem *NodeAllocator::freelist = NULL;
klock_t NodeAllocator::lock;

void *NodeAllocator::allocate() {
	SpinLock::acquire(&lock);
	if(freelist == NULL) {
		size_t i,oldCount;
		oldCount = nodeArray.getObjCount();
		if(!nodeArray.extend()) {
			SpinLock::release(&lock);
			return NULL;
		}
		for(i = oldCount; i < nodeArray.getObjCount(); i++) {
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
