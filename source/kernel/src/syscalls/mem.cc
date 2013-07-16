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
#include <sys/task/fd.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
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

int sysc_mmap(sThread *t,sIntrptStackFrame *stack) {
	uintptr_t addr = SYSC_ARG1(stack);
	size_t byteCount = SYSC_ARG2(stack);
	size_t loadCount = SYSC_ARG3(stack);
	int prot = SYSC_ARG4(stack);
	int flags = SYSC_ARG5(stack);
	int fd = SYSC_ARG6(stack);
	off_t binOffset = SYSC_ARG7(stack);
	pid_t pid = t->proc->pid;
	sFile *f = NULL;
	sVMRegion *vm;
	int res;

	/* check args */
	if(byteCount == 0 || loadCount > byteCount)
		SYSC_ERROR(stack,-EINVAL);
	if(flags & MAP_FIXED) {
		if(addr & (PAGE_SIZE - 1))
			SYSC_ERROR(stack,-EINVAL);
		if(addr == 0 || addr + byteCount < addr || addr + byteCount > INTERP_TEXT_BEGIN)
			SYSC_ERROR(stack,-EINVAL);
	}
	if(flags & MAP_TLS) {
		if(thread_getTLSRegion(t) != NULL)
			SYSC_ERROR(stack,-EINVAL);
		thread_reserveFrames(BYTES_2_PAGES(byteCount));
	}
	if(fd != -1) {
		/* get file */
		f = fd_request(fd);
		if(f == NULL)
			SYSC_ERROR(stack,-EBADF);
	}

	/* add region */
	res = vmm_map(pid,addr,byteCount,loadCount,prot,flags,f,binOffset,&vm);
	if(f)
		fd_release(f);
	/* save tls-region-number */
	if(flags & MAP_TLS) {
		if(res == 0)
			thread_setTLSRegion(t,vm);
		thread_discardFrames();
	}
	if(res < 0)
		SYSC_ERROR(stack,res);

	vmm_getRegRange(pid,vm,&addr,0,true);
	SYSC_RET1(stack,addr);
}

int sysc_mprotect(sThread *t,sIntrptStackFrame *stack) {
	pid_t pid = t->proc->pid;
	void *addr = (void*)SYSC_ARG1(stack);
	uint prot = (uint)SYSC_ARG2(stack);
	int res;

	if(!(prot & (PROT_WRITE | PROT_READ | PROT_EXEC)))
		SYSC_ERROR(stack,-EINVAL);

	res = vmm_regctrl(pid,(uintptr_t)addr,prot);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int sysc_munmap(sThread *t,sIntrptStackFrame *stack) {
	void *virt = (void*)SYSC_ARG1(stack);
	sVMRegion *reg = vmm_getRegion(t->proc,(uintptr_t)virt);
	if(reg == NULL)
		SYSC_ERROR(stack,-ENOENT);
	vmm_remove(t->proc->pid,reg);
	SYSC_RET1(stack,0);
}

int sysc_regaddphys(sThread *t,sIntrptStackFrame *stack) {
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
	/* reserve frames if we don't want to use contiguous physical memory */
	if(!physCpy && !align)
		thread_reserveFrames(BYTES_2_PAGES(bytes));

	addr = vmm_addPhys(pid,&physCpy,bytes,align,true);
	if(addr == 0)
		SYSC_ERROR(stack,-ENOMEM);
	*phys = physCpy;
	SYSC_RET1(stack,addr);
}
