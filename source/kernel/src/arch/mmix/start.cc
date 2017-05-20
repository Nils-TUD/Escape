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

#include <dbg/console.h>
#include <mem/virtmem.h>
#include <task/elf.h>
#include <task/proc.h>
#include <task/terminator.h>
#include <task/thread.h>
#include <task/uenv.h>
#include <assert.h>
#include <boot.h>
#include <common.h>
#include <cpu.h>
#include <util.h>

EXTERN_C uintptr_t bspstart(BootInfo *bootinfo,uint64_t *stackBegin,uint64_t *rss);

uintptr_t bspstart(BootInfo *bootinfo,uint64_t *stackBegin,uint64_t *rss) {
	Boot::start(bootinfo);

	/* give the process some stack pages */
	Thread *t = Thread::getRunning();
	if(!t->reserveFrames(INITIAL_STACK_PAGES * 2))
		Util::panic("Not enough mem for initloader-stack");
	t->addInitialStack();
	t->discardFrames();

	/* load initloader */
	OpenFile *file;
	pid_t pid = t->getProc()->getPid();
	ELF::StartupInfo info;
	if(VFS::openPath(pid,VFS_EXEC | VFS_READ,0,"/sys/boot/initloader",NULL,&file) < 0)
		Util::panic("Unable to open initloader");
	if(ELF::load(file,&info) < 0)
		Util::panic("Unable to load initloader");
	if(!UEnv::setupProc(0,0,NULL,0,&info,info.progEntry,-1))
		Util::panic("Unable to setup initloader");
	file->close();
	*stackBegin = info.stackBegin;
	*rss = DIR_MAP_AREA | (t->getKernelStack() * PAGE_SIZE);
	return info.progEntry;
}
