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
#include <sys/task/filedesc.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/mem/virtmem.h>
#include <sys/syscalls.h>
#include <sys/boot.h>
#include <esc/fsinterface.h>
#include <string.h>
#include <errno.h>

int Syscalls::chgsize(Thread *t,IntrptStackFrame *stack) {
	ssize_t count = SYSC_ARG1(stack);
	if(count > 0) {
		if(!t->reserveFrames(count))
			SYSC_ERROR(stack,-ENOMEM);
	}
	size_t oldEnd = t->getProc()->getVM()->growData(count);
	if(count > 0)
		t->discardFrames();
	SYSC_RET1(stack,oldEnd);
}

int Syscalls::mmap(Thread *t,IntrptStackFrame *stack) {
	uintptr_t addr = SYSC_ARG1(stack);
	size_t byteCount = SYSC_ARG2(stack);
	size_t loadCount = SYSC_ARG3(stack);
	int prot = SYSC_ARG4(stack);
	int flags = SYSC_ARG5(stack);
	int fd = SYSC_ARG6(stack);
	off_t binOffset = SYSC_ARG7(stack);
	OpenFile *f = NULL;

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
		if(t->getTLSRegion() != NULL)
			SYSC_ERROR(stack,-EINVAL);
		if(!t->reserveFrames(BYTES_2_PAGES(byteCount)))
			SYSC_ERROR(stack,-ENOMEM);
	}
	if(fd != -1) {
		/* get file */
		f = FileDesc::request(fd);
		if(f == NULL)
			SYSC_ERROR(stack,-EBADF);
	}

	/* add region */
	VMRegion *vm;
	int res = t->getProc()->getVM()->map(addr,byteCount,loadCount,prot,flags,f,binOffset,&vm);
	if(f)
		FileDesc::release(f);
	/* save tls-region-number */
	if(flags & MAP_TLS) {
		if(res == 0)
			t->setTLSRegion(vm);
		t->discardFrames();
	}
	if(res < 0)
		SYSC_ERROR(stack,res);

	t->getProc()->getVM()->getRegRange(vm,&addr,0,true);
	SYSC_RET1(stack,addr);
}

int Syscalls::mprotect(Thread *t,IntrptStackFrame *stack) {
	void *addr = (void*)SYSC_ARG1(stack);
	uint prot = (uint)SYSC_ARG2(stack);

	if(!(prot & (PROT_WRITE | PROT_READ | PROT_EXEC)))
		SYSC_ERROR(stack,-EINVAL);

	int res = t->getProc()->getVM()->regctrl((uintptr_t)addr,prot);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::munmap(Thread *t,IntrptStackFrame *stack) {
	void *virt = (void*)SYSC_ARG1(stack);
	VMRegion *reg = t->getProc()->getVM()->getRegion((uintptr_t)virt);
	if(reg == NULL)
		SYSC_ERROR(stack,-ENOENT);
	t->getProc()->getVM()->remove(reg);
	SYSC_RET1(stack,0);
}

int Syscalls::regaddphys(Thread *t,IntrptStackFrame *stack) {
	uintptr_t *phys = (uintptr_t*)SYSC_ARG1(stack);
	size_t bytes = SYSC_ARG2(stack);
	size_t align = SYSC_ARG3(stack);
	uintptr_t physCpy = *phys;

	if(!PageDir::isInUserSpace((uintptr_t)phys,sizeof(uintptr_t)))
		SYSC_ERROR(stack,-EFAULT);

	/* ensure that its allowed to map this area (if the address is specified) */
	if(physCpy && !PhysMem::canMap(physCpy,bytes))
		SYSC_ERROR(stack,-EFAULT);
	/* reserve frames if we don't want to use contiguous physical memory */
	if(!physCpy && !align) {
		if(!t->reserveFrames(BYTES_2_PAGES(bytes)))
			SYSC_ERROR(stack,-ENOMEM);
	}

	uintptr_t addr = t->getProc()->getVM()->addPhys(&physCpy,bytes,align,true);
	if(!physCpy && !align)
		t->discardFrames();
	if(addr == 0)
		SYSC_ERROR(stack,-ENOMEM);
	*phys = physCpy;
	SYSC_RET1(stack,addr);
}
