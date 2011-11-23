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
#include <sys/mem/swapmap.h>
#include <sys/mem/paging.h>
#include <sys/boot.h>
#include <sys/video.h>

#include "tkheap.h"
#include "tdynarray.h"
#include "tpaging.h"
#include "tproc.h"
#include "tmm.h"
#include "tsched.h"
#include "tsllist.h"
#include "tstring.h"
#include "tvfsnode.h"
#include "tsignals.h"
#include "tringbuffer.h"
#include "tesccodes.h"
#include "tswapmap.h"
#include "tregion.h"
#include "tvmm.h"
#include "tshm.h"
#include "thashmap.h"
#include "tctype.h"
#include "tvmreg.h"
#include "tvmfree.h"
/* TODO find a better solution */
#ifdef __mmix__
#include "arch/mmix/taddrspace.h"
#endif

/* make gcc happy */
void bspstart(sBootInfo *mbp);
uintptr_t smpstart(void);
void apstart(void);

void bspstart(sBootInfo *bootinfo) {
	/* init the kernel */
	boot_start(bootinfo);
	vid_setTargets(TARGET_SCREEN | TARGET_LOG);
	/* TODO find a better solution */
#ifdef __i386__
	paging_gdtFinished();
#endif

	/* start tests */
	/* swapmap (needed for swapmap tests) */
	vid_printf("Initializing Swapmap...");
	swmap_init(256 * K);
	vid_printf("\033[co;2]%|s\033[co]","DONE");

#ifdef __mmix__
	test_register(&tModAddrSpace);
#endif
	test_register(&tModVMReg);
	test_register(&tModVMFree);
	test_register(&tModCtype);
	test_register(&tModMM);
	test_register(&tModDynArray);
	test_register(&tModPaging);
	test_register(&tModProc);
	test_register(&tModHashMap);
	test_register(&tModKHeap);
	test_register(&tModRegion);
	test_register(&tModRBuffer);
	test_register(&tModShm);
	test_register(&tModSLList);
	test_register(&tModSched);
	test_register(&tModString);
	test_register(&tModVFSn);
	test_register(&tModSignals);
	test_register(&tModEscCodes);
	test_register(&tModSwapMap);
	test_register(&tModVmm);
	test_start();

	/* stay here */
	while(1);
}

uintptr_t smpstart(void) {
	/* not used */
	return 0;
}

void apstart(void) {
	/* not used */
}
