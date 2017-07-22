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
#include <dbg/kb.h>
#include <mem/pagedir.h>
#include <mem/swapmap.h>
#include <sys/test.h>
#include <task/proc.h>
#include <task/terminator.h>
#include <task/thread.h>
#include <boot.h>
#include <common.h>
#include <log.h>
#include <util.h>

/* TODO find a better solution */
#if defined(__mmix__)
extern sTestModule tModAddrSpace;
#endif
extern sTestModule tModVMReg;
extern sTestModule tModVMFree;
extern sTestModule tModMM;
extern sTestModule tModDynArray;
extern sTestModule tModPaging;
extern sTestModule tModProc;
extern sTestModule tModKHeap;
extern sTestModule tModRegion;
extern sTestModule tModSched;
extern sTestModule tModVFSn;
extern sTestModule tModSwapMap;
extern sTestModule tModVmm;
extern sTestModule tModPmemAreas;
extern sTestModule tModCache;

EXTERN_C void unittest_run();
EXTERN_C void unittest_start();

void unittest_start() {
	/* swapmap (needed for swapmap tests) */
	SwapMap::init(256 * 1024);

	/* start the terminator */
	Proc::startThread((uintptr_t)&Terminator::start,0,NULL);

	/* register callback */
	Boot::setUnittests(unittest_run);
}

void unittest_run() {
	/* first unmap all userspace stuff */
	Proc::removeRegions(Proc::getRunning(),true);

#if defined(__mmix__)
	test_register(&tModAddrSpace);
#endif
	test_register(&tModVMReg);
	test_register(&tModVMFree);
	test_register(&tModMM);
	test_register(&tModDynArray);
	test_register(&tModPaging);
	test_register(&tModProc);
	test_register(&tModKHeap);
	test_register(&tModRegion);
	test_register(&tModSched);
	test_register(&tModVFSn);
	test_register(&tModSwapMap);
	test_register(&tModVmm);
	test_register(&tModPmemAreas);
	test_register(&tModCache);
	test_start();

	/* stay here */
	tprintf("\nPress any key to continue...\n");
	while(1) {
		Keyboard::get(NULL,Keyboard::EVENT_PRESS,true);
		Console::start("");
	}
}
