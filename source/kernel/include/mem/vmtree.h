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
#include <esc/col/dlisttreap.h>
#include <esc/col/slist.h>
#include <mem/shfiles.h>
#include <vfs/fileid.h>
#include <mutex.h>

class Region;
class VirtMem;
class OStream;

struct VMRegion : public esc::DListTreapNode<uintptr_t> {
    explicit VMRegion(Region *_reg,uintptr_t _virt)
    	: esc::DListTreapNode<uintptr_t>(_virt), fileuse(), reg(_reg) {
    }

    virtual bool matches(uintptr_t key);

    /**
     * @return the virtual address of this region
     */
    uintptr_t virt() const {
    	return key();
    }
    /**
     * Sets the virtual address. Note that this is ONLY possible if you are sure that it doesn't
     * change the order in the tree!
     */
    void virt(uintptr_t _virt) {
    	key(_virt);
    }

    virtual void print(OStream &os) {
		os.writef("virt=%p region=%p\n",virt(),reg);
    }

    ShFiles::FileUsage *fileuse;
	Region *reg;
};

class VMTree {
public:
	typedef esc::DListTreap<VMRegion>::iterator iterator;
	typedef esc::DListTreap<VMRegion>::const_iterator const_iterator;

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
	static VMTree *reqTree() {
		regMutex.down();
		return regList;
	}

	/**
	 * Releases the linked list of trees again
	 */
	static void relTree() {
		regMutex.up();
	}

	/**
	 * @return the virtmem object it belongs to
	 */
	VirtMem *getVM() const {
		return virtmem;
	}
	/**
	 * @return the next tree
	 */
	VMTree *getNext() const {
		return next;
	}

	/**
	 * @return the beginning of the vm-region list
	 */
	iterator begin() {
		return regs.begin();
	}
	const_iterator cbegin() const {
		return regs.cbegin();
	}
	/**
	 * @return the end of the vm-region list
	 */
	iterator end() {
		return regs.end();
	}
	const_iterator cend() const {
		return regs.cend();
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
	VMRegion *getByAddr(uintptr_t addr) const {
		return regs.find(addr);
	}

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

private:
	VirtMem *virtmem;
	esc::DListTreap<VMRegion> regs;
	VMTree *next;

	/* mutex for accessing/changing the list of all vm-regions */
	static Mutex regMutex;
	static VMTree *regList;
	static VMTree *regListEnd;
};
