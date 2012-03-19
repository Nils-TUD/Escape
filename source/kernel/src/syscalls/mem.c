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
#include <sys/boot.h>
#include <esc/fsinterface.h>
#include <string.h>
#include <errno.h>

int sysc_chgsize(sThread *t,sIntrptStackFrame *stack) {
	ssize_t count = SYSC_ARG1(stack);
	pid_t pid = t->proc->pid;
	size_t oldEnd;
	if(count > 0)
		thread_reserveFrames(count);
	oldEnd = vmm_grow(pid,t->proc->dataAddr,count);
	if(count > 0)
		thread_discardFrames();
	SYSC_RET1(stack,oldEnd);
}

int sysc_regadd(sThread *t,sIntrptStackFrame *stack) {
	sBinDesc binCpy;
	const sBinDesc *bin = (sBinDesc*)SYSC_ARG1(stack);
	off_t binOffset = SYSC_ARG2(stack);
	size_t byteCount = SYSC_ARG3(stack);
	size_t loadCount = SYSC_ARG4(stack);
	uint type = SYSC_ARG5(stack);
	pid_t pid = t->proc->pid;
	sVMRegion *vm;
	uintptr_t start;
	int res;

	/* copy the bin-desc, for the case that bin is not accessible */
	if(bin)
		memcpy(&binCpy,bin,sizeof(sBinDesc));

	/* check type */
	switch(type) {
		case REG_TEXT:
		case REG_DATA:
		case REG_RODATA:
		case REG_SHLIBDATA:
		case REG_SHLIBTEXT:
		case REG_SHM:
		case REG_DEVICE:
		case REG_TLS:
			/* the user can't create a new stack here */
			break;
		default:
			SYSC_ERROR(stack,-EPERM);
			break;
	}

	/* add region */
	res = vmm_add(pid,bin ? &binCpy : NULL,binOffset,byteCount,loadCount,type,&vm);
	if(res < 0)
		SYSC_ERROR(stack,res);
	/* save tls-region-number */
	if(type == REG_TLS)
		thread_setTLSRegion(t,vm);
	vmm_getRegRange(pid,vm,&start,0,true);
	SYSC_RET1(stack,start);
}

int sysc_regctrl(sThread *t,sIntrptStackFrame *stack) {
	pid_t pid = t->proc->pid;
	uintptr_t addr = SYSC_ARG1(stack);
	uint prot = (uint)SYSC_ARG2(stack);
	ulong flags = 0;
	int res;

	if(!(prot & (PROT_WRITE | PROT_READ)))
		SYSC_ERROR(stack,-EINVAL);
	if(prot & PROT_WRITE)
		flags |= RF_WRITABLE;

	res = vmm_regctrl(pid,addr,flags);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int sysc_mapmod(sThread *t,sIntrptStackFrame *stack) {
	char namecpy[256];
	const char *name = (const char*)SYSC_ARG1(stack);
	size_t sizecpy,*size = (size_t*)SYSC_ARG2(stack);
	pid_t pid = t->proc->pid;
	uintptr_t addr,phys;

	if(!paging_isInUserSpace((uintptr_t)size,sizeof(size_t)))
		SYSC_ERROR(stack,-EFAULT);

	strnzcpy(namecpy,name,sizeof(namecpy));
	phys = boot_getModuleRange(namecpy,&sizecpy);
	if(phys == 0)
		SYSC_ERROR(stack,-ENOENT);

	addr = vmm_addPhys(pid,&phys,sizecpy,1,false);
	if(addr == 0)
		SYSC_ERROR(stack,-ENOMEM);
	*size = sizecpy;
	SYSC_RET1(stack,addr);
}

int sysc_mapphys(sThread *t,sIntrptStackFrame *stack) {
	uintptr_t *phys = (uintptr_t*)SYSC_ARG1(stack);
	size_t bytes = SYSC_ARG2(stack);
	size_t align = SYSC_ARG3(stack);
	pid_t pid = t->proc->pid;
	uintptr_t addr,physCpy = *phys;

	if(!paging_isInUserSpace((uintptr_t)phys,sizeof(uintptr_t)))
		SYSC_ERROR(stack,-EFAULT);

	/* ensure that its allowed to map this area (if the address is specified) */
	if(physCpy && !pmem_canMap(physCpy,bytes))
		SYSC_ERROR(stack,-EFAULT);

	addr = vmm_addPhys(pid,&physCpy,bytes,align,true);
	if(addr == 0)
		SYSC_ERROR(stack,-ENOMEM);
	*phys = physCpy;
	SYSC_RET1(stack,addr);
}

int sysc_shmcrt(sThread *t,sIntrptStackFrame *stack) {
	char namecpy[MAX_SHAREDMEM_NAME + 1];
	const char *name = (const char*)SYSC_ARG1(stack);
	size_t byteCount = SYSC_ARG2(stack);
	pid_t pid = t->proc->pid;
	int res;

	if(byteCount == 0)
		SYSC_ERROR(stack,-EINVAL);
	strnzcpy(namecpy,name,sizeof(namecpy));

	res = shm_create(pid,namecpy,BYTES_2_PAGES(byteCount));
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res * PAGE_SIZE);
}

int sysc_shmjoin(sThread *t,sIntrptStackFrame *stack) {
	char namecpy[MAX_SHAREDMEM_NAME + 1];
	const char *name = (const char*)SYSC_ARG1(stack);
	pid_t pid = t->proc->pid;
	int res;

	strnzcpy(namecpy,name,sizeof(namecpy));
	res = shm_join(pid,namecpy);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res * PAGE_SIZE);
}

int sysc_shmleave(sThread *t,sIntrptStackFrame *stack) {
	char namecpy[MAX_SHAREDMEM_NAME + 1];
	const char *name = (const char*)SYSC_ARG1(stack);
	pid_t pid = t->proc->pid;
	int res;

	strnzcpy(namecpy,name,sizeof(namecpy));
	res = shm_leave(pid,namecpy);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_shmdel(sThread *t,sIntrptStackFrame *stack) {
	char namecpy[MAX_SHAREDMEM_NAME + 1];
	const char *name = (const char*)SYSC_ARG1(stack);
	pid_t pid = t->proc->pid;
	int res;

	strnzcpy(namecpy,name,sizeof(namecpy));
	res = shm_destroy(pid,namecpy);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
