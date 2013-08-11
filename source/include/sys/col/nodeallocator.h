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

#pragma once

#include <sys/common.h>
#include <sys/mem/dynarray.h>

class SListItem;
template<class T>
struct IListNode;

/* The idea is to provide a very fast node-allocation and -deallocation for the indirect
 * single-linked-list that is used in kernel. It is used at very many places, so that its really
 * worth it. To achieve that, we use the dynamic-array and dedicate an area in virtual-memory to
 * the nodes. This area is extended on demand and all nodes are put in a freelist (they have a
 * next-element anyway). This is much quicker than using the heap everytime a node is allocated or
 * free'd. That means, if we put no heap-allocated data in the sll (so data, that does exist
 * anyway), using it does basically cost nothing :) */

class NodeAllocator {
	template<class T>
	friend struct IListNode;

	NodeAllocator() = delete;

	static void *allocate();
	static void free(void *ptr);

	static DynArray nodeArray;
	static SListItem *freelist;
	static klock_t lock;
};
