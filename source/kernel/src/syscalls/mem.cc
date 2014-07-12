/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <task/filedesc.h>
#include <mem/pagedir.h>
#include <mem/kheap.h>
#include <mem/virtmem.h>
#include <syscalls.h>
#include <boot.h>
#include <esc/fsinterface.h>
#include <string.h>
#include <errno.h>

int Syscalls::chgsize(Thread *t,IntrptStackFrame *stack) {
	ssize_t count = SYSC_ARG1(stack);
	if(EXPECT_TRUE(count > 0)) {
		if(EXPECT_FALSE(!t->reserveFrames(count)))
			SYSC_RET1(stack,0);
	}
	size_t oldEnd = t->getProc()->getVM()->growData(count);
	if(EXPECT_TRUE(count > 0))
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

#if DISABLE_DEMLOAD
	flags |= MAP_POPULATE;
#endif

	/* check args */
	if(EXPECT_FALSE(byteCount == 0 || loadCount > byteCount))
		SYSC_ERROR(stack,-EINVAL);
	if(flags & ~MAP_USER_FLAGS)
		SYSC_ERROR(stack,-EINVAL);

	if(flags & MAP_FIXED) {
		if(EXPECT_FALSE(addr & (PAGE_SIZE - 1)))
			SYSC_ERROR(stack,-EINVAL);
		if(EXPECT_FALSE(addr == 0 || addr + byteCount < addr || addr + byteCount > INTERP_TEXT_BEGIN))
			SYSC_ERROR(stack,-EINVAL);
	}

	if((flags & (MAP_POPULATE | MAP_NOSWAP)) == MAP_POPULATE) {
		if(EXPECT_FALSE(!t->reserveFrames(BYTES_2_PAGES(byteCount))))
			SYSC_ERROR(stack,-ENOMEM);
	}

	/* get file */
	if(EXPECT_TRUE(fd != -1)) {
		f = FileDesc::request(t->getProc(),fd);
		if(EXPECT_FALSE(f == NULL))
			SYSC_ERROR(stack,-EBADF);
	}

	/* map memory */
	VMRegion *vm;
	int res;
	do {
		/* we have to retry it if it failed because another process is not available atm. */
		/* TODO that is a really stupid solution */
		res = t->getProc()->getVM()->map(&addr,byteCount,loadCount,prot,flags,f,binOffset,&vm);
		/* give the other process a chance to finish */
		if(res == -EBUSY)
			Thread::switchNoSigs();
	}
	while(res == -EBUSY);

	/* release file */
	if(EXPECT_TRUE(f))
		FileDesc::release(f);

	t->discardFrames();
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);

	SYSC_RET1(stack,addr);
}

int Syscalls::mprotect(Thread *t,IntrptStackFrame *stack) {
	void *addr = (void*)SYSC_ARG1(stack);
	uint prot = (uint)SYSC_ARG2(stack);

	if(EXPECT_FALSE(!(prot & (PROT_WRITE | PROT_READ | PROT_EXEC))))
		SYSC_ERROR(stack,-EINVAL);

	int res = t->getProc()->getVM()->protect((uintptr_t)addr,prot);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::munmap(Thread *t,IntrptStackFrame *stack) {
	void *virt = (void*)SYSC_ARG1(stack);
	int res = t->getProc()->getVM()->unmap((uintptr_t)virt);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::mlock(Thread *t,IntrptStackFrame *stack) {
	void *virt = (void*)SYSC_ARG1(stack);
	int flags = (int)SYSC_ARG2(stack);
	if(flags & ~MAP_NOSWAP)
		SYSC_ERROR(stack,-EINVAL);

	/* reserve frames in advance with swapping? */
	if(~flags & MAP_NOSWAP) {
		uintptr_t start,end;
		int res = t->getProc()->getVM()->getRegRange((uintptr_t)virt,&start,&end);
		if(res < 0)
			SYSC_ERROR(stack,res);

		size_t pages = BYTES_2_PAGES(end - start);
		if(!t->reserveFrames(pages,true))
			SYSC_ERROR(stack,-ENOMEM);
	}

	int res = t->getProc()->getVM()->lock((uintptr_t)virt,flags);
	t->discardFrames();
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::mlockall(Thread *t,IntrptStackFrame *stack) {
	int res = t->getProc()->getVM()->lockall();
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::mmapphys(Thread *t,IntrptStackFrame *stack) {
	uintptr_t *phys = (uintptr_t*)SYSC_ARG1(stack);
	size_t bytes = SYSC_ARG2(stack);
	size_t align = SYSC_ARG3(stack);
	int flags = SYSC_ARG4(stack);
	uintptr_t physCpy = *phys;

	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)phys,sizeof(uintptr_t))))
		SYSC_ERROR(stack,-EFAULT);

	/* ensure that its allowed to map this area (if the address is specified) */
	if(EXPECT_FALSE((flags & MAP_PHYS_MAP) && !PhysMem::canMap(physCpy,bytes)))
		SYSC_ERROR(stack,-EFAULT);
	/* reserve frames if we don't want to use contiguous physical memory */
	if(!(flags & MAP_PHYS_MAP) && !align) {
		if(EXPECT_FALSE(!t->reserveFrames(BYTES_2_PAGES(bytes))))
			SYSC_ERROR(stack,-ENOMEM);
	}

	uintptr_t addr = t->getProc()->getVM()->mapphys(&physCpy,bytes,align,flags);
	if(EXPECT_TRUE(!(flags & MAP_PHYS_MAP) && !align))
		t->discardFrames();
	if(EXPECT_FALSE(addr == 0))
		SYSC_ERROR(stack,-ENOMEM);
	*phys = physCpy;
	SYSC_RET1(stack,addr);
}
