/**
 * $Id: start.c 900 2011-06-02 20:18:17Z nasmussen $
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <sys/boot.h>
#include <sys/util.h>
#include <assert.h>

static uint8_t initloader[] = {
#if DEBUGGING
#	include "../../../../build/mmix-debug/user_initloader.dump"
#else
#	include "../../../../build/mmix-release/user_initloader.dump"
#endif
};

int main(const sBootInfo *bootinfo,uint64_t *stackBegin,uint64_t *rss) {
	sThread *t;
	sStartupInfo info;

	boot_init(bootinfo,true);

	/* give the process some stack pages */
	t = thread_getRunning();
	t->stackRegions[0] = vmm_add(t->proc,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
			INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACKUP);
	assert(t->stackRegions[0] >= 0);
	t->stackRegions[1] = vmm_add(t->proc,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
			INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACK);
	assert(t->stackRegions[1] >= 0);

	/* load initloader */
	if(elf_loadFromMem(initloader,sizeof(initloader),&info) < 0)
		util_panic("Unable to load initloader");
	*stackBegin = info.stackBegin;
	*rss = DIR_MAPPED_SPACE | (t->kstackFrame * PAGE_SIZE);
	vid_printf("rss=%lx *rss=%lx\n",rss,*rss);
	paging_dbg_printCur(0);
	vmm_dbg_print(t->proc);
	return info.progEntry;
}
