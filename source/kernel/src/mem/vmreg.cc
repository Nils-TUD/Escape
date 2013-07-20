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
#include <sys/mem/vmreg.h>
#include <sys/mem/region.h>
#include <sys/mem/cache.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/video.h>
#include <sys/mutex.h>
#include <assert.h>

/**
 * We use a treap (combination of binary tree and heap) to be able to find vm-regions by an address
 * very quickly. To be able to walk through all vm-regions quickly as well, we maintain a linked
 * list of this vm-regions as well.
 */

static void vmreg_doRemove(sVMRegion **p,sVMRegion *reg);
static void vmreg_doPrint(const sVMRegion *n,int layer);

/* mutex for accessing/changing the list of all vm-regions */
static mutex_t regMutex;
static sVMRegTree *regList;
static sVMRegTree *regListEnd;

void vmreg_addTree(pid_t pid,sVMRegTree *tree) {
	mutex_aquire(&regMutex);
	if(regListEnd)
		regListEnd->next = tree;
	else
		regList = tree;
	regListEnd = tree;
	tree->pid = pid;
	tree->next = NULL;
	tree->begin = NULL;
	tree->end = NULL;
	tree->root = NULL;
	tree->priority = 314159265;
	mutex_release(&regMutex);
}

void vmreg_remTree(sVMRegTree *tree) {
	sVMRegTree *t,*p;
	p = NULL;
	mutex_aquire(&regMutex);
	for(t = regList; t != NULL; p = t, t = t->next) {
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
	mutex_release(&regMutex);
}

sVMRegTree *vmreg_reqTree(void) {
	mutex_aquire(&regMutex);
	return regList;
}

void vmreg_relTree(void) {
	mutex_release(&regMutex);
}

bool vmreg_available(sVMRegTree *tree,uintptr_t addr,size_t size) {
	sVMRegion *vm;
	for(vm = tree->begin; vm != NULL; vm = vm->next) {
		uintptr_t end = vm->virt + ROUND_PAGE_UP(vm->reg->byteCount);
		if(OVERLAPS(addr,addr + size,vm->virt,end))
			return false;
	}
	return true;
}

sVMRegion *vmreg_getByAddr(sVMRegTree *tree,uintptr_t addr) {
	sVMRegion *vm;
	for(vm = tree->root; vm != NULL; ) {
		if(addr >= vm->virt && addr < vm->virt + ROUND_PAGE_UP(vm->reg->byteCount))
			return vm;
		if(addr < vm->virt)
			vm = vm->left;
		else
			vm = vm->right;
	}
	return NULL;
}

sVMRegion *vmreg_getByReg(sVMRegTree *tree,sRegion *reg) {
	sVMRegion *vm;
	for(vm = tree->begin; vm != NULL; vm = vm->next) {
		if(vm->reg == reg)
			return vm;
	}
	return NULL;
}

sVMRegion *vmreg_add(sVMRegTree *tree,sRegion *reg,uintptr_t addr) {
	/* find a place for a new node. we want to insert it by priority, so find the first
	 * node that has <= priority */
	sVMRegion *p,**q,**l,**r;
	for(p = tree->root, q = &tree->root; p && p->priority < tree->priority; p = *q) {
		if(addr < p->virt)
			q = &p->left;
		else
			q = &p->right;
	}
	/* create new node */
	*q = (sVMRegion*)Cache::alloc(sizeof(sVMRegion));
	if(!*q)
		return NULL;
	/* we have a reference to that file now. we'll release it on unmap */
	if(reg->file)
		vfs_incRefs(reg->file);
	/* fibonacci hashing to spread the priorities very even in the 32-bit room */
	(*q)->priority = tree->priority;
	tree->priority += 0x9e3779b9;	/* floor(2^32 / phi), with phi = golden ratio */
	(*q)->reg = reg;
	(*q)->virt = addr;
	/* At this point we want to split the binary search tree p into two parts based on the
	 * given key, forming the left and right subtrees of the new node q. The effect will be
	 * as if key had been inserted before all of p’s nodes. */
	l = &(*q)->left;
	r = &(*q)->right;
	while(p) {
		if(addr < p->virt) {
			*r = p;
			r = &p->left;
			p = *r;
		}
		else {
			*l = p;
			l = &p->right;
			p = *l;
		}
	}
	*l = *r = NULL;
	p = *q;
	/* insert at the end of the linked list */
	if(tree->end)
		tree->end->next = p;
	else
		tree->begin = p;
	tree->end = p;
	p->next = NULL;
	return p;
}

void vmreg_remove(sVMRegTree *tree,sVMRegion *reg) {
	sVMRegion **p,*r,*prev;
	/* find the position where reg is stored */
	for(p = &tree->root; *p && *p != reg; ) {
		if(reg->virt < (*p)->virt)
			p = &(*p)->left;
		else
			p = &(*p)->right;
	}
	assert(*p);
	/* remove from tree */
	vmreg_doRemove(p,reg);

	/* remove from linked list */
	prev = NULL;
	for(r = tree->begin; r != NULL; prev = r, r = r->next) {
		if(r == reg) {
			if(prev)
				prev->next = r->next;
			else
				tree->begin = r->next;
			if(!r->next)
				tree->end = prev;
			break;
		}
	}
	/* close file */
	if(reg->reg->file)
		vfs_closeFile(tree->pid,reg->reg->file);
	Cache::free(reg);
}

static void vmreg_doRemove(sVMRegion **p,sVMRegion *reg) {
	/* two childs */
	if(reg->left && reg->right) {
		/* rotate with left */
		if(reg->left->priority < reg->right->priority) {
			sVMRegion *t = reg->left;
			reg->left = t->right;
			t->right = reg;
			*p = t;
			vmreg_doRemove(&t->right,reg);
		}
		/* rotate with right */
		else {
			sVMRegion *t = reg->right;
			reg->right = t->left;
			t->left = reg;
			*p = t;
			vmreg_doRemove(&t->left,reg);
		}
	}
	/* one child: replace us with our child */
	else if(reg->left)
		*p = reg->left;
	else if(reg->right)
		*p = reg->right;
	/* no child: simply remove us from parent */
	else
		*p = NULL;
}

void vmreg_print(sVMRegTree *tree) {
	vmreg_doPrint(tree->root,0);
	vid_printf("\n");
}

static void vmreg_doPrint(const sVMRegion *n,int layer) {
	if(n) {
		vid_printf("prio=%08x, addr=%p\n",n->priority,n->virt);
		vid_printf("%*s\\-(l) ",layer * 2,"");
		vmreg_doPrint(n->left,layer + 1);
		vid_printf("\n");
		vid_printf("%*s\\-(r) ",layer * 2,"");
		vmreg_doPrint(n->right,layer + 1);
	}
}
