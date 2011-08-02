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
#include <sys/task/proc.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/mem/sharedmem.h>
#include <sys/mem/vmm.h>
#include <sys/syscalls/mem.h>
#include <sys/syscalls.h>
#include <string.h>
#include <errors.h>

int sysc_changeSize(sIntrptStackFrame *stack) {
	ssize_t count = SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	ssize_t oldEnd;
	vmreg_t rno = RNO_DATA;
	/* if there is no data-region, maybe we're the dynamic linker that has a dldata-region */
	if(!vmm_exists(p->pid,rno)) {
		/* if so, grow that region instead */
		rno = vmm_getDLDataReg(p->pid);
		if(rno == -1)
			SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
	}
	if((oldEnd = vmm_grow(p->pid,rno,count)) < 0)
		SYSC_ERROR(stack,oldEnd);
	SYSC_RET1(stack,oldEnd);
}

int sysc_addRegion(sIntrptStackFrame *stack) {
	sBinDesc binCpy;
	const sBinDesc *bin = (sBinDesc*)SYSC_ARG1(stack);
	off_t binOffset = SYSC_ARG2(stack);
	size_t byteCount = SYSC_ARG3(stack);
	size_t loadCount = SYSC_ARG4(stack);
	uint type = SYSC_ARG5(stack);
	sThread *t = thread_getRunning();
	sProc *p = t->proc;
	vmreg_t rno = -1;
	uintptr_t start;

	/* copy the bin-desc, for the case that bin is not accessible */
	if(bin)
		memcpy(&binCpy,bin,sizeof(sBinDesc));

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
		if(vmm_exists(p->pid,rno))
			SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}

	/* add region */
	rno = vmm_add(p->pid,bin ? &binCpy : NULL,binOffset,byteCount,loadCount,type);
	if(rno < 0)
		SYSC_ERROR(stack,rno);
	/* save tls-region-number */
	if(type == REG_TLS)
		thread_setTLSRegion(t,rno);
	vmm_getRegRange(p->pid,rno,&start,0);
	SYSC_RET1(stack,start);
}

int sysc_setRegProt(sIntrptStackFrame *stack) {
	sProc *p = proc_getRunning();
	uintptr_t addr = SYSC_ARG1(stack);
	uint prot = (uint)SYSC_ARG2(stack);
	ulong flags = 0;
	vmreg_t rno;
	int res;

	if(!(prot & (PROT_WRITE | PROT_READ)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(prot & PROT_WRITE)
		flags |= RF_WRITABLE;

	rno = vmm_getRegionOf(p->pid,addr);
	if(rno < 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vmm_setRegProt(p->pid,rno,flags);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int sysc_mapPhysical(sIntrptStackFrame *stack) {
	uintptr_t *phys = (uintptr_t*)SYSC_ARG1(stack);
	size_t bytes = SYSC_ARG2(stack);
	size_t align = SYSC_ARG3(stack);
	sProc *p = proc_getRunning();
	uintptr_t addr,physCpy = *phys;

	if(!paging_isInUserSpace((uintptr_t)phys,sizeof(uintptr_t)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* trying to map memory in kernel area? */
#ifdef __i386__
	size_t pages = BYTES_2_PAGES(bytes);
	/* TODO is this ok? */
	/* TODO I think we should check here whether it is in a used-region, according to multiboot-memmap */
	if(physCpy &&
			OVERLAPS(physCpy,physCpy + pages,KERNEL_P_ADDR,KERNEL_P_ADDR + PAGE_SIZE * PT_ENTRY_COUNT))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
#endif

	addr = vmm_addPhys(p->pid,&physCpy,bytes,align);
	if(addr == 0)
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
	*phys = physCpy;
	SYSC_RET1(stack,addr);
}

int sysc_createSharedMem(sIntrptStackFrame *stack) {
	char namecpy[MAX_SHAREDMEM_NAME + 1];
	const char *name = (const char*)SYSC_ARG1(stack);
	size_t byteCount = SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	int res;

	if(byteCount == 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	strncpy(namecpy,name,sizeof(namecpy));
	namecpy[sizeof(namecpy) - 1] = '\0';

	res = shm_create(p->pid,namecpy,BYTES_2_PAGES(byteCount));
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res * PAGE_SIZE);
}

int sysc_joinSharedMem(sIntrptStackFrame *stack) {
	char namecpy[MAX_SHAREDMEM_NAME + 1];
	const char *name = (const char*)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	int res;

	strncpy(namecpy,name,sizeof(namecpy));
	namecpy[sizeof(namecpy) - 1] = '\0';
	res = shm_join(p->pid,namecpy);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res * PAGE_SIZE);
}

int sysc_leaveSharedMem(sIntrptStackFrame *stack) {
	char namecpy[MAX_SHAREDMEM_NAME + 1];
	const char *name = (const char*)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	int res;

	strncpy(namecpy,name,sizeof(namecpy));
	namecpy[sizeof(namecpy) - 1] = '\0';
	res = shm_leave(p->pid,namecpy);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_destroySharedMem(sIntrptStackFrame *stack) {
	char namecpy[MAX_SHAREDMEM_NAME + 1];
	const char *name = (const char*)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	int res;

	strncpy(namecpy,name,sizeof(namecpy));
	namecpy[sizeof(namecpy) - 1] = '\0';
	res = shm_destroy(p->pid,namecpy);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
