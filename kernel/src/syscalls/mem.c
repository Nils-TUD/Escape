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

#include <common.h>
#include <task/proc.h>
#include <mem/paging.h>
#include <mem/kheap.h>
#include <mem/sharedmem.h>
#include <mem/vmm.h>
#include <syscalls/mem.h>
#include <syscalls.h>
#include <errors.h>

void sysc_changeSize(sIntrptStackFrame *stack) {
	s32 count = SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	s32 oldEnd;
	if((oldEnd = vmm_grow(p,RNO_DATA,count)) < 0)
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
	SYSC_RET1(stack,oldEnd);
}

void sysc_mapPhysical(sIntrptStackFrame *stack) {
	u32 phys = SYSC_ARG1(stack);
	u32 bytes = SYSC_ARG2(stack);
	u32 pages = BYTES_2_PAGES(bytes);
	sProc *p = proc_getRunning();
	u32 addr;

	/* trying to map memory in kernel area? */
	/* TODO is this ok? */
	/* TODO I think we should check here wether it is in a used-region, according to multiboot-memmap */
	if(OVERLAPS(phys,phys + pages,KERNEL_P_ADDR,KERNEL_P_ADDR + PAGE_SIZE * PT_ENTRY_COUNT))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	addr = vmm_addPhys(p,phys,bytes);
	if(addr == 0)
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
	SYSC_RET1(stack,addr);
}

void sysc_createSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	u32 byteCount = SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	s32 res;

	if(!sysc_isStringReadable(name) || byteCount == 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = shm_create(p,name,BYTES_2_PAGES(byteCount));
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res * PAGE_SIZE);
}

void sysc_joinSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	s32 res;

	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = shm_join(p,name);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res * PAGE_SIZE);
}

void sysc_leaveSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	s32 res;

	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = shm_leave(p,name);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_destroySharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	s32 res;

	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = shm_destroy(p,name);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
