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
#include <sys/dbg/console.h>
#include <sys/task/thread.h>
#include <sys/task/elf.h>
#include <sys/mem/vmm.h>
#include <sys/cpu.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <assert.h>

static A_ALIGNED(8) uint8_t initloader[] = {
#if DEBUGGING
#	include "../../../../build/mmix-debug/user_initloader.dump"
#else
#	include "../../../../build/mmix-release/user_initloader.dump"
#endif
};

uintptr_t bspstart(sBootInfo *bootinfo,uint64_t *stackBegin,uint64_t *rss) {
	sThread *t;
	sStartupInfo info;

	boot_start(bootinfo);

	/* give the process some stack pages */
	t = thread_getRunning();
	if(!thread_reserveFrames(INITIAL_STACK_PAGES * 2))
		util_panic("Not enough mem for initloader-stack");
	thread_addInitialStack(t);
	thread_discardFrames();

	/* load initloader */
	if(elf_loadFromMem(initloader,sizeof(initloader),&info) < 0)
		util_panic("Unable to load initloader");
	*stackBegin = info.stackBegin;
	*rss = DIR_MAPPED_SPACE | (t->archAttr.kstackFrame * PAGE_SIZE);
	return info.progEntry;
}
