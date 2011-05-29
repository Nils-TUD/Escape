/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#ifndef SYSCALLS_MEM_H_
#define SYSCALLS_MEM_H_

#include <sys/intrpt.h>

/* protection-flags */
#define PROT_READ			1
#define PROT_WRITE			2

/**
 * Changes the process-size
 *
 * @param ssize_t number of pages
 * @return ssize_t the previous number of text+data pages
 */
void sysc_changeSize(sIntrptStackFrame *stack);

/**
 * Adds a region to the current process at the appropriate virtual address (depending on
 * existing regions and the region-type) from given binary.
 *
 * @param sBinDesc* the binary (may be NULL; means: no demand-loading possible)
 * @param uintptr_t the offset of the region in the binary (for demand-loading)
 * @param size_t the number of bytes of the region
 * @param uint the region-type (see REG_*)
 * @return void* the address of the region on success, NULL on failure
 */
void sysc_addRegion(sIntrptStackFrame *stack);

/**
 * Changes the protection of the region denoted by the given address.
 *
 * @param uintptr_t the virtual address
 * @param uint the new protection-setting (PROT_*)
 * @return int 0 on success
 */
void sysc_setRegProt(sIntrptStackFrame *stack);

/**
 * Maps physical memory in the virtual user-space
 *
 * @param uintptr_t physical address
 * @param size_t number of bytes
 * @return void* the start-address
 */
void sysc_mapPhysical(sIntrptStackFrame *stack);

/**
 * Creates a shared-memory region
 *
 * @param char* the name
 * @param size_t number of bytes
 * @return intptr_t the address on success, negative error-code otherwise
 */
void sysc_createSharedMem(sIntrptStackFrame *stack);

/**
 * Joines a shared-memory region
 *
 * @param char* the name
 * @return intptr_t the address on success, negative error-code otherwise
 */
void sysc_joinSharedMem(sIntrptStackFrame *stack);

/**
 * Leaves a shared-memory region
 *
 * @param char* the name
 * @return int 0 on success, negative error-code otherwise
 */
void sysc_leaveSharedMem(sIntrptStackFrame *stack);

/**
 * Destroys a shared-memory region
 *
 * @param char* the name
 * @return int 0 on success, negative error-code otherwise
 */
void sysc_destroySharedMem(sIntrptStackFrame *stack);

#endif /* SYSCALLS_MEM_H_ */
