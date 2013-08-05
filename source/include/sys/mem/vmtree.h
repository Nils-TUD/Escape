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
#include <sys/mutex.h>

class Region;
class VirtMem;
class OStream;

struct VMRegion {
	Region *reg;
	uintptr_t virt;
	/* for the treap */
	uint32_t priority;
	VMRegion *left;
	VMRegion *right;
	/* for the linked list */
	VMRegion *next;
};

class VMTree {
public:
	/**
	 * Does NOT initialize the object
	 */
	VMTree() {
	}

	/**
	 * Initializes and adds the given tree into the linked list of all vmreg-trees
	 *
	 * @param vm the virtual memory it belongs to
	 * @param tree the tree to add
	 */
	static void addTree(VirtMem *vm,VMTree *tree);

	/**
	 * Removes the given tree from the linked list of all vmreg-trees. Assumes that its empty.
	 *
	 * @param tree the tree
	 */
	static void remTree(VMTree *tree);

	/**
	 * Requests the linked list of all trees (locks it)
	 *
	 * @return the list of trees
	 */
	static VMTree *reqTree();

	/**
	 * Releases the linked list of trees again
	 */
	static void relTree();

	/**
	 * @return the virtmem object it belongs to
	 */
	VirtMem *getVM() const {
		return virtmem;
	}
	/**
	 * @return the first vm-region in this tree
	 */
	VMRegion *first() const {
		return begin;
	}
	/**
	 * @return the next tree
	 */
	VMTree *getNext() const {
		return next;
	}

	/**
	 * Checks whether <addr>..<addr>+<size> is still available.
	 *
	 * @param addr the start-address
	 * @param size the size of the region
	 * @return true if it's still free
	 */
	bool available(uintptr_t addr,size_t size) const;

	/**
	 * Finds a vm-region in the tree by an address. That is, it walks through the binary tree,
	 * which is pretty fast.
	 *
	 * @param addr the address to search for
	 * @return the region or NULL if not found
	 */
	VMRegion *getByAddr(uintptr_t addr) const;

	/**
	 * Finds a vm-region in the tree by a region. That is, it walks through the linked list,
	 * which takes a bit longer than searching for an address.
	 *
	 * @param reg the region to search for.
	 * @return the region or NULL if not found
	 */
	VMRegion *getByReg(Region *reg) const;

	/**
	 * Adds a new vm-region to the tree.
	 *
	 * @param reg the region
	 * @param addr the address to put the region at
	 * @return the created vm-region
	 */
	VMRegion *add(Region *reg,uintptr_t addr);

	/**
	 * Removes the given vm-region from the tree
	 *
	 * @param reg the vm-region
	 */
	void remove(VMRegion *reg);

	/**
	 * Prints the tree
	 *
	 * @param os the output-stream
	 */
	void print(OStream &os) const;

private:
	static void doRemove(VMRegion **p,VMRegion *reg);
	static void doPrint(OStream &os,const VMRegion *n,int layer);

	VirtMem *virtmem;
	/* the linked list */
	VMRegion *begin;
	VMRegion *end;
	/* the tree */
	VMRegion *root;
	uint32_t priority;
	VMTree *next;

	/* mutex for accessing/changing the list of all vm-regions */
	static mutex_t regMutex;
	static VMTree *regList;
	static VMTree *regListEnd;
};

inline VMTree *VMTree::reqTree() {
	Mutex::acquire(&regMutex);
	return regList;
}

inline void VMTree::relTree() {
	Mutex::release(&regMutex);
}
