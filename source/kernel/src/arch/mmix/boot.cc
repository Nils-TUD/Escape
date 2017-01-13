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

#include <arch/mmix/mem/addrspace.h>
#include <esc/util.h>
#include <mem/cache.h>
#include <mem/copyonwrite.h>
#include <mem/dynarray.h>
#include <mem/pagedir.h>
#include <mem/virtmem.h>
#include <task/elf.h>
#include <task/proc.h>
#include <task/sched.h>
#include <task/smp.h>
#include <task/terminator.h>
#include <task/thread.h>
#include <task/timer.h>
#include <task/uenv.h>
#include <vfs/node.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <boot.h>
#include <common.h>
#include <config.h>
#include <cpu.h>
#include <log.h>
#include <string.h>
#include <util.h>
#include <video.h>

static const BootTask tasks[] = {
	{"Parsing bootinfo...",Boot::parseBootInfo},
	{"Parsing cmdline...",Boot::parseCmdline},
	{"Initializing physical memory-management...",PhysMem::init},
	{"Initializing address spaces...",AddressSpace::init},
	{"Initializing paging...",PageDir::init},
	{"Preinit processes...",Proc::preinit},
	{"Initializing dynarray...",DynArray::init},
	{"Initializing SMP...",SMP::init},
	{"Initializing timer...",Timer::init},
	{"Initializing VFS...",VFS::init},
	{"Initializing scheduler...",Sched::init},
	{"Initializing processes...",Proc::init},
	{"Creating MB module files...",Boot::createModFiles},
	{"Start logging to VFS...",Log::vfsIsReady},
	{"Initializing interrupts...",Interrupts::init},
};
BootTaskList Boot::taskList(tasks,ARRAY_SIZE(tasks));

static Boot::Module mods[BOOT_PROG_MAX];
static Boot::MemMap mmap;
static BootInfo *binfo;
static int initPhase = 0;

void Boot::archStart(void *nfo) {
	binfo = (BootInfo*)nfo;
	CPU::setSpeed(binfo->cpuHz);
}

void Boot::parseBootInfo() {
	info.cmdLine = (char*)binfo->progs[0].command;
	info.modCount = binfo->progCount - 1;
	info.mods = mods;
	for(size_t i = 1; i < binfo->progCount; ++i) {
		info.mods[i - 1].name = (char*)binfo->progs[i].command;
		info.mods[i - 1].phys = binfo->progs[i].start & ~DIR_MAP_AREA;
		info.mods[i - 1].size = binfo->progs[i].size;
		info.mods[i - 1].virt = binfo->progs[i].start;
	}
	/* mark everything behind the modules as free */
	Module *last = info.mods + info.modCount - 1;
	mmap.baseAddr = esc::Util::round_page_up(last->phys + last->size);
	mmap.size = binfo->memSize - mmap.baseAddr;
	mmap.type = 1;
	info.mmapCount = 1;
	info.mmap = &mmap;
}

int Boot::init(A_UNUSED IntrptStackFrame *stack) {
	if(initPhase > 3)
		return -EEXIST;

	if(unittests != NULL)
		unittests();

	/* we have to do the forking here because we have to leave to userland afterwards (the child
	 * gets our kernel stack). furthermore, we can only do one fork at a time */

	switch(initPhase) {
		case 0:
			if(Proc::clone(P_KERNEL) == 0) {
				/* the child shouldn't try again */
				return -EEXIST;
			}
			break;

		case 1:
			/* start the swapper-process. it will never return */
			if(PhysMem::canSwap())
				Proc::startKProc("[swapper]",&PhysMem::swapper);
			break;

		case 2:
			/* and the terminator */
			Proc::startKProc("[terminator]",&Terminator::start);
			break;

		case 3: {
			Thread *t = Thread::getRunning();
			/* we can remove all user regions now */
			Proc::removeRegions(t->getProc()->getPid(),true);

			/* stay in kernel idling */
			t->makeIdle();
			thread_idle();
			A_UNREACHED;
			break;
		}
	}
	initPhase++;
	return 0;
}
