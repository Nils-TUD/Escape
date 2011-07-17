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
#include <sys/arch/i586/task/thread.h>
#include <sys/arch/i586/mem/paging.h>
#include <sys/arch/i586/gdt.h>
#include <sys/task/thread.h>
#include <sys/task/elf.h>
#include <sys/mem/vmm.h>
#include <sys/mem/paging.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <assert.h>

static uint8_t initloader[] = {
#if DEBUGGING
#	include "../../../../build/i586-debug/user_initloader.dump"
#else
#	include "../../../../build/i586-release/user_initloader.dump"
#endif
};

uintptr_t bspstart(sBootInfo *mbp,uint32_t magic) {
	UNUSED(magic);
	sThread *t;
	sStartupInfo info;

	/* init the kernel */
	boot_init(mbp,true);

	/* load initloader */
	if(elf_loadFromMem(initloader,sizeof(initloader),&info) < 0)
		util_panic("Unable to load initloader");
	/* give the process some stack pages */
	t = thread_getRunning();
	thread_addInitialStack(t);
	return info.progEntry;
}

void apstart(void) {
	while(1);
	gdt_init_ap();
	paging_activate();
	thread_initialSwitch();
}
