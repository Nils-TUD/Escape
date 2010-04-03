/**
 * $Id$
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

#include <common.h>
#include <machine/gdt.h>
#include <machine/fpu.h>
#include <machine/intrpt.h>
#include <machine/cpu.h>
#include <machine/timer.h>
#include <machine/serial.h>
#include <mem/pmem.h>
#include <mem/paging.h>
#include <mem/kheap.h>
#include <mem/vmm.h>
#include <mem/cow.h>
#include <mem/swapmap.h>
#include <task/sched.h>
#include <task/elf.h>
#include <task/proc.h>
#include <task/signals.h>
#include <vfs/vfs.h>
#include <vfs/info.h>
#include <vfs/request.h>
#include <vfs/driver.h>
#include <vfs/real.h>
#include <util.h>
#include <debug.h>
#include <multiboot.h>
#include <kevent.h>
#include <video.h>
#include <test.h>
#include <log.h>
#include <string.h>

#include "tkheap.h"
#include "tpaging.h"
#include "tproc.h"
#include "tmm.h"
#include "tsched.h"
#include "tsllist.h"
#include "tstring.h"
#include "tvfs.h"
#include "tvfsnode.h"
#include "tsignals.h"
#include "tringbuffer.h"
#include "tesccodes.h"
#include "tvfslist.h"
#include "tswapmap.h"
#include "tregion.h"
#include "tvmm.h"

s32 main(sMultiBoot *mbp,u32 magic) {
	UNUSED(magic);

	/* the first thing we've to do is set up the page-dir and page-table for the kernel and so on
	 * and "correct" the GDT */
	paging_init();
	gdt_init();
	mboot_init(mbp);

	/* init video and serial-ports */
	vid_init();
	ser_init();

	vid_printf("GDT exchanged, paging enabled, video initialized");
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	mboot_dbg_print();

	/* mm */
	vid_printf("Initializing physical memory-management...");
	mm_init(mboot_getInfo());
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* paging */
	vid_printf("Initializing paging...");
	paging_mapKernelSpace();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* fpu */
	vid_printf("Initializing FPU...");
	fpu_init();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* vfs */
	vid_printf("Initializing VFS...");
	vfs_init();
	vfsinfo_init();
	vfsreq_init();
	vfsdrv_init();
	vfsr_init();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* processes */
	vid_printf("Initializing process-management...");
	proc_init();
	sched_init();
	/* the process and thread-stuff has to be ready, too ... */
	/* but no logging to vfs in test-mode */
	/*log_vfsIsReady();*/
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* vmm */
	vid_printf("Initializing virtual memory management...");
	vmm_init();
	cow_init();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* idt */
	vid_printf("Initializing IDT...");
	intrpt_init();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* timer */
	vid_printf("Initializing timer...");
	timer_init();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* signals */
	vid_printf("Initializing signal-handling...");
	sig_init();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* kevents */
	vid_printf("Initializing KEvents...");
	kev_init();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* cpu */
	vid_printf("Detecting CPU...");
	cpu_detect();
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	vid_printf("%d free frames (%d KiB)\n",mm_getFreeFrmCount(MM_DMA | MM_DEF),
			mm_getFreeFrmCount(MM_DMA | MM_DEF) * PAGE_SIZE / K);

	/* swapmap (needed for swapmap tests) */
	vid_printf("Initializing Swapmap...");
	swmap_init(256 * K);
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	/* start tests */
	test_register(&tModMM);
	test_register(&tModPaging);
	test_register(&tModProc);
	test_register(&tModKHeap);
	test_register(&tModSched);
	test_register(&tModSLList);
	test_register(&tModString);
	test_register(&tModVFS);
	test_register(&tModVFSn);
	test_register(&tModSignals);
	test_register(&tModRBuffer);
	test_register(&tModEscCodes);
	test_register(&tModVFSList);
	test_register(&tModSwapMap);
	test_register(&tModRegion);
	test_register(&tModVmm);
	test_start();


	/* stay here */
	while(1);

	return 0;
}
