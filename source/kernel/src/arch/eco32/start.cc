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
#include <sys/task/elf.h>
#include <sys/task/thread.h>
#include <sys/mem/paging.h>
#include <sys/mem/virtmem.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <assert.h>

EXTERN_C uintptr_t bspstart(BootInfo *bootinfo);

static A_ALIGNED(4) uint8_t initloader[] = {
#if DEBUGGING
#	include "../../../../build/eco32-debug/user_initloader.dump"
#else
#	include "../../../../build/eco32-release/user_initloader.dump"
#endif
};

uintptr_t bspstart(BootInfo *bootinfo) {
	Thread *t;
	ELF::StartupInfo info;

	Boot::start(bootinfo);

	/* load initloader */
	if(ELF::loadFromMem(initloader,sizeof(initloader),&info) < 0)
		Util::panic("Unable to load initloader");
	t = Thread::getRunning();
	if(!t->reserveFrames(INITIAL_STACK_PAGES))
		Util::panic("Not enough mem for initloader-stack");
	t->addInitialStack();
	t->discardFrames();
	/* we have to set the kernel-stack for the first process */
	PageDir::tlbSet(0,KERNEL_STACK,(t->getKernelStack() * PAGE_SIZE) | 0x3);
	return info.progEntry;
}
