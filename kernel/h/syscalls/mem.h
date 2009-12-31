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

#include <machine/intrpt.h>

/**
 * Changes the process-size
 *
 * @param u32 number of pages
 * @return u32 the previous number of text+data pages
 */
void sysc_changeSize(sIntrptStackFrame *stack);

/**
 * Maps physical memory in the virtual user-space
 *
 * @param u32 physical address
 * @param u32 number of bytes
 * @return void* the start-address
 */
void sysc_mapPhysical(sIntrptStackFrame *stack);

/**
 * Creates a shared-memory region
 *
 * @param char* the name
 * @param u32 number of bytes
 * @return s32 the address on success, negative error-code otherwise
 */
void sysc_createSharedMem(sIntrptStackFrame *stack);

/**
 * Joines a shared-memory region
 *
 * @param char* the name
 * @return s32 the address on success, negative error-code otherwise
 */
void sysc_joinSharedMem(sIntrptStackFrame *stack);

/**
 * Leaves a shared-memory region
 *
 * @param char* the name
 * @return s32 the address on success, negative error-code otherwise
 */
void sysc_leaveSharedMem(sIntrptStackFrame *stack);

/**
 * Destroys a shared-memory region
 *
 * @param char* the name
 * @return s32 the address on success, negative error-code otherwise
 */
void sysc_destroySharedMem(sIntrptStackFrame *stack);

#endif /* SYSCALLS_MEM_H_ */
