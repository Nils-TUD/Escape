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
#include <sys/mem/vmm.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <assert.h>

static A_ALIGNED(4) uint8_t initloader[] = {
#if DEBUGGING
#	include "../../../../build/eco32-debug/user_initloader.dump"
#else
#	include "../../../../build/eco32-release/user_initloader.dump"
#endif
};

int main(const sBootInfo *bootinfo) {
	sThread *t;
	sStartupInfo info;

	boot_init(bootinfo,true);

	/* load initloader */
	if(elf_loadFromMem(initloader,sizeof(initloader),&info) < 0)
		util_panic("Unable to load initloader");
	t = thread_getRunning();
	/* give the process some stack pages */
	t->stackRegions[0] = vmm_add(t->proc,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
			INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACK);
	assert(t->stackRegions[0] >= 0);
	/* we have to set the kernel-stack for the first process */
	tlb_set(0,KERNEL_STACK,(t->kstackFrame * PAGE_SIZE) | 0x3);
	return info.progEntry;
}
