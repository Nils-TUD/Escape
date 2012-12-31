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
#include <sys/mem/region.h>

typedef struct sVMRegion {
	sRegion *reg;
	uintptr_t virt;
	/* file for the binary (valid if >= 0) */
	sFile *binFile;
	/* for the treap */
	uint32_t priority;
	struct sVMRegion *left;
	struct sVMRegion *right;
	/* for the linked list */
	struct sVMRegion *next;
} sVMRegion;

typedef struct sVMRegTree {
	pid_t pid;
	/* the linked list */
	sVMRegion *begin;
	sVMRegion *end;
	/* the tree */
	sVMRegion *root;
	uint32_t priority;
	struct sVMRegTree *next;
} sVMRegTree;

/**
 * Initializes and adds the given tree into the linked list of all vmreg-trees
 *
 * @param pid the process-id
 * @param tree the tree to add
 */
void vmreg_addTree(pid_t pid,sVMRegTree *tree);

/**
 * Removes the given tree from the linked list of all vmreg-trees. Assumes that its empty.
 *
 * @param tree the tree
 */
void vmreg_remTree(sVMRegTree *tree);

/**
 * Requests the linked list of all trees (locks it)
 *
 * @return the list of trees
 */
sVMRegTree *vmreg_reqTree(void);

/**
 * Releases the linked list of trees again
 */
void vmreg_relTree(void);

/**
 * Finds a vm-region in the given tree by an address. That is, it walks through the binary tree,
 * which is pretty fast.
 *
 * @param tree the tree
 * @param addr the address to search for
 * @return the region or NULL if not found
 */
sVMRegion *vmreg_getByAddr(sVMRegTree *tree,uintptr_t addr);

/**
 * Finds a vm-region in the given tree by a region. That is, it walks through the linked list,
 * which takes a bit longer than searching for an address.
 *
 * @param tree the tree
 * @param reg the region to search for.
 * @return the region or NULL if not found
 */
sVMRegion *vmreg_getByReg(sVMRegTree *tree,sRegion *reg);

/**
 * Adds a new vm-region to the given tree.
 *
 * @param tree the tree
 * @param reg the region
 * @param addr the address to put the region at
 * @return the created vm-region
 */
sVMRegion *vmreg_add(sVMRegTree *tree,sRegion *reg,uintptr_t addr);

/**
 * Removes the given vm-region from the given tree
 *
 * @param tree the tree
 * @param reg the vm-region
 */
void vmreg_remove(sVMRegTree *tree,sVMRegion *reg);

/**
 * Prints the given tree
 *
 * @param tree the tree
 */
void vmreg_print(sVMRegTree *tree);
