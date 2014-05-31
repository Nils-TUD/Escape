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

#include <sys/common.h>
#include <sys/task/elf.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/terminator.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/virtmem.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <assert.h>

EXTERN_C uintptr_t bspstart(BootInfo *bootinfo,uint32_t cpuSpeed,uintptr_t *usp);

uintptr_t bspstart(BootInfo *bootinfo,uint32_t cpuSpeed,uintptr_t *usp) {
	Boot::start(bootinfo);

	CPU::setSpeed(cpuSpeed);

	/* start idle-thread */
	Proc::startThread((uintptr_t)&thread_idle,T_IDLE,NULL);

	/* TODO */
#if 0
	/* start the swapper-thread. it will never return */
	if(PhysMem::canSwap())
		Proc::startThread((uintptr_t)&PhysMem::swapper,0,NULL);
#endif
	Proc::startThread((uintptr_t)&Terminator::start,0,NULL);

	/* load initloader */
	ELF::StartupInfo info;
	if(ELF::load("/system/boot/initloader",&info) < 0)
		Util::panic("Unable to load initloader");
	Thread *t = Thread::getRunning();
	if(!t->reserveFrames(INITIAL_STACK_PAGES))
		Util::panic("Not enough mem for initloader-stack");
	*usp = t->addInitialStack() + INITIAL_STACK_PAGES * PAGE_SIZE - 4;
	t->discardFrames();
	/* we have to set the kernel-stack for the first process */
	PageDir::tlbSet(0,KERNEL_STACK,(t->getKernelStack() * PAGE_SIZE) | 0x3);
	return info.progEntry;
}
