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

#include <mem/pagedir.h>
#include <mem/virtmem.h>
#include <sys/arch.h>
#include <task/elf.h>
#include <task/proc.h>
#include <task/terminator.h>
#include <task/thread.h>
#include <vfs/vfs.h>
#include <assert.h>
#include <boot.h>
#include <common.h>
#include <util.h>

EXTERN_C void bspstart(BootInfo *bootinfo,uint32_t cpuSpeed);
EXTERN_C uintptr_t procstart(uintptr_t *usp);

void bspstart(BootInfo *bootinfo,uint32_t cpuSpeed) {
	Boot::start(bootinfo);

	CPU::setSpeed(cpuSpeed);

	/* we have to set the kernel-stack for the first process */
	Thread *t = Thread::getRunning();
	PageDir::tlbSet(0,KERNEL_STACK,(t->getKernelStack() * PAGE_SIZE) | 0x3);
}

uintptr_t procstart(uintptr_t *usp) {
	/* start init process */
	if(Proc::clone(P_KERNEL) == 0) {
		ELF::StartupInfo info;
		Thread *t = Thread::getRunning();
		pid_t pid = t->getProc()->getPid();

		/* load initloader */
		OpenFile *file;
		if(VFS::openPath(pid,VFS_EXEC | VFS_READ,0,"/sys/boot/initloader",NULL,&file) < 0)
			Util::panic("Unable to open initloader");
		if(ELF::load(file,&info) < 0)
			Util::panic("Unable to load initloader");
		file->close(pid);

		/* give the process some stack pages */
		if(!t->reserveFrames(INITIAL_STACK_PAGES))
			Util::panic("Not enough mem for initloader-stack");
		*usp = t->addInitialStack() + INITIAL_STACK_PAGES * PAGE_SIZE - WORDSIZE * 3;
		t->discardFrames();
		return info.progEntry;
	}

	/* start the swapper-process. it will never return */
	if(PhysMem::canSwap())
		Proc::startKProc("[swapper]",&PhysMem::swapper);
	/* and the terminator */
	Proc::startKProc("[terminator]",&Terminator::start);

	thread_idle();
	A_UNREACHED;
}
