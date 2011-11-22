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

#ifndef VMREG_H_
#define VMREG_H_

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
	sVMRegion *begin;
	sVMRegion *end;
	sVMRegion *root;
	uint32_t priority;
	struct sVMRegTree *next;
} sVMRegTree;

void vmreg_addTree(pid_t pid,sVMRegTree *tree);
void vmreg_remTree(sVMRegTree *tree);
sVMRegTree *vmreg_reqTree(void);
void vmreg_relTree(void);
sVMRegion *vmreg_getByAddr(sVMRegTree *tree,uintptr_t addr);
sVMRegion *vmreg_getByReg(sVMRegTree *tree,sRegion *reg);
sVMRegion *vmreg_add(sVMRegTree *tree,sRegion *reg,uintptr_t addr);
void vmreg_remove(sVMRegTree *tree,sVMRegion *reg);
void vmreg_print(sVMRegTree *tree);

#endif /* VMREG_H_ */
