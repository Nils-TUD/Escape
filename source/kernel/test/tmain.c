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

#include <sys/common.h>
#include <sys/mem/swapmap.h>
#include <sys/boot.h>
#include <sys/video.h>

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
#include "tswapmap.h"
#include "tregion.h"
#include "tvmm.h"
#include "tshm.h"
#include "thashmap.h"
/* TODO find a better solution */
#ifdef __mmix__
#include "arch/mmix/taddrspace.h"
#endif

int main(sBootInfo *bootinfo,uint32_t magic) {
	UNUSED(magic);

	/* init the kernel */
	boot_init(bootinfo,false);

	/* start tests */
#ifdef __mmix__
	test_register(&tModAddrSpace);
	test_register(&tModPaging);
	test_register(&tModKHeap);
	test_register(&tModString);
	test_register(&tModRBuffer);
	test_register(&tModEscCodes);
	test_register(&tModHashMap);
	test_register(&tModSLList);
#else
	/* swapmap (needed for swapmap tests) */
	vid_printf("Initializing Swapmap...");
	swmap_init(256 * K);
	vid_printf("\033[co;2]%|s\033[co]","DONE");

	test_register(&tModMM);
	test_register(&tModPaging);
	/*test_register(&tModProc);*/
	test_register(&tModKHeap);
	test_register(&tModSched);
	test_register(&tModString);
	test_register(&tModVFS);
	test_register(&tModVFSn);
	test_register(&tModSignals);
	test_register(&tModRBuffer);
	test_register(&tModEscCodes);
	test_register(&tModSwapMap);
	test_register(&tModRegion);
	test_register(&tModVmm);
	test_register(&tModShm);
	test_register(&tModHashMap);
	test_register(&tModSLList);
#endif
	test_start();

	/* stay here */
	while(1);

	return 0;
}
