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

#include <sys/common.h>
#include <sys/task/proc.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/mem/sharedmem.h>
#include <sys/mem/vmm.h>
#include <sys/syscalls/mem.h>
#include <sys/syscalls.h>
#include <errors.h>

void sysc_changeSize(sIntrptStackFrame *stack) {
	ssize_t count = SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	ssize_t oldEnd;
	tVMRegNo rno = RNO_DATA;
	/* if there is no data-region, maybe we're the dynamic linker that has a dldata-region */
	if(!vmm_exists(p,rno)) {
		/* if so, grow that region instead */
		rno = vmm_getDLDataReg(p);
		if(rno == -1)
			SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
	}
	if((oldEnd = vmm_grow(p,rno,count)) < 0)
		SYSC_ERROR(stack,oldEnd);
	SYSC_RET1(stack,oldEnd);
}

void sysc_addRegion(sIntrptStackFrame *stack) {
	sBinDesc *bin = (sBinDesc*)SYSC_ARG1(stack);
	uintptr_t binOffset = SYSC_ARG2(stack);
	size_t byteCount = SYSC_ARG3(stack);
	size_t loadCount = SYSC_ARG4(stack);
	uint type = SYSC_ARG5(stack);
	sThread *t = thread_getRunning();
	sProc *p = t->proc;
	tVMRegNo rno = -1;
	uintptr_t start;

	/* check bin */
	if(bin != NULL && !paging_isRangeUserReadable((uintptr_t)bin,sizeof(sBinDesc)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* check type */
	switch(type) {
		case REG_TEXT:
			rno = RNO_TEXT;
			break;
		case REG_DATA:
			rno = RNO_DATA;
			break;
		case REG_RODATA:
			rno = RNO_RODATA;
			break;
		case REG_SHLIBDATA:
		case REG_SHLIBTEXT:
		case REG_SHM:
		case REG_DEVICE:
		case REG_TLS:
			/* the user can't create a new stack here */
			break;
		default:
			SYSC_ERROR(stack,ERR_INVALID_ARGS);
			break;
	}

	/* check if the region-type does already exist */
	if(rno >= 0) {
		if(vmm_exists(p,rno))
			SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}

	/* add region */
	rno = vmm_add(p,bin,binOffset,byteCount,loadCount,type);
	if(rno < 0)
		SYSC_ERROR(stack,rno);
	/* save tls-region-number */
	if(type == REG_TLS)
		t->tlsRegion = rno;
	vmm_getRegRange(p,rno,&start,0);
	SYSC_RET1(stack,start);
}

void sysc_setRegProt(sIntrptStackFrame *stack) {
	sProc *p = proc_getRunning();
	uintptr_t addr = SYSC_ARG1(stack);
	uint prot = (uint)SYSC_ARG2(stack);
	uint flags = 0;
	tVMRegNo rno;
	int res;

	if(!(prot & (PROT_WRITE | PROT_READ)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(prot & PROT_WRITE)
		flags |= RF_WRITABLE;

	rno = vmm_getRegionOf(p,addr);
	if(rno < 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vmm_setRegProt(p,rno,flags);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

void sysc_mapPhysical(sIntrptStackFrame *stack) {
	uintptr_t *phys = (uintptr_t*)SYSC_ARG1(stack);
	size_t bytes = SYSC_ARG2(stack);
	size_t align = SYSC_ARG3(stack);
	size_t pages = BYTES_2_PAGES(bytes);
	sProc *p = proc_getRunning();
	uintptr_t addr;

	/* check phys */
	if(!paging_isRangeUserReadable((uintptr_t)phys,sizeof(uintptr_t)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* trying to map memory in kernel area? */
	/* TODO is this ok? */
	/* TODO I think we should check here wether it is in a used-region, according to multiboot-memmap */
	if(*phys &&
			OVERLAPS(*phys,*phys + pages,KERNEL_P_ADDR,KERNEL_P_ADDR + PAGE_SIZE * PT_ENTRY_COUNT))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	addr = vmm_addPhys(p,phys,bytes,align);
	if(addr == 0)
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
	SYSC_RET1(stack,addr);
}

void sysc_createSharedMem(sIntrptStackFrame *stack) {
	char *name = (char*)SYSC_ARG1(stack);
	size_t byteCount = SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	int res;

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
	int res;

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
	int res;

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
	int res;

	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = shm_destroy(p,name);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
