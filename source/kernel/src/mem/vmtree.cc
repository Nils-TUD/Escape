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

#include <mem/cache.h>
#include <mem/region.h>
#include <mem/vmtree.h>
#include <task/proc.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <assert.h>
#include <common.h>
#include <mutex.h>
#include <video.h>

/**
 * We use a treap (combination of binary tree and heap) to be able to find vm-regions by an address
 * very quickly. To be able to walk through all vm-regions quickly as well, we maintain a linked
 * list of this vm-regions as well.
 */

Mutex VMTree::regMutex;
VMTree *VMTree::regList;
VMTree *VMTree::regListEnd;

bool VMRegion::matches(uintptr_t k) {
    return k >= virt() && k < virt() + ROUND_PAGE_UP(reg->getByteCount());
}

void VMTree::addTree(VirtMem *vm,VMTree *tree) {
	LockGuard<Mutex> g(&regMutex);
	if(regListEnd)
		regListEnd->next = tree;
	else
		regList = tree;
	regListEnd = tree;
	tree->virtmem = vm;
	tree->next = NULL;
	tree->regs.clear();
}

void VMTree::remTree(VMTree *tree) {
	LockGuard<Mutex> g(&regMutex);
	VMTree *p = NULL;
	for(VMTree *t = regList; t != NULL; p = t, t = t->next) {
		if(t == tree) {
			if(p)
				p->next = t->next;
			else
				regList = t->next;
			if(t == regListEnd)
				regListEnd = p;
			break;
		}
	}
}

bool VMTree::available(uintptr_t addr,size_t size) const {
	for(auto vm = regs.cbegin(); vm != regs.cend(); ++vm) {
		uintptr_t endaddr = vm->virt() + ROUND_PAGE_UP(vm->reg->getByteCount());
		if(OVERLAPS(addr,addr + size,vm->virt(),endaddr))
			return false;
	}
	return true;
}

VMRegion *VMTree::getByReg(Region *reg) const {
	for(auto vm = regs.cbegin(); vm != regs.cend(); ++vm) {
		if(vm->reg == reg)
			return const_cast<VMRegion*>(&*vm);
	}
	return NULL;
}

VMRegion *VMTree::add(Region *reg,uintptr_t addr) {
	VMRegion *vm = new VMRegion(reg,addr);
	if(!vm)
		return NULL;
	/* we have a reference to that file now. we'll release it on unmap */
	if(reg->getFile())
		reg->getFile()->incRefs();
	regs.insert(vm);
	return vm;
}

void VMTree::remove(VMRegion *reg) {
	/* close file */
	if(reg->reg->getFile())
		reg->reg->getFile()->close(virtmem->getProc()->getPid());
	regs.remove(reg);
	delete reg;
}
