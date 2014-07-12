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

#include <common.h>
#include <arch/x86/serial.h>
#include <arch/x86/gdt.h>
#include <task/proc.h>
#include <task/thread.h>
#include <task/smp.h>
#include <task/uenv.h>
#include <task/terminator.h>
#include <mem/pagedir.h>
#include <boot.h>
#include <util.h>

/* make gcc happy */
EXTERN_C void bspstart(void *mbp);
EXTERN_C uintptr_t smpstart(uintptr_t *usp);
EXTERN_C void apstart();
static void idlestart();
extern SpinLock aplock;
extern ulong proc0TLPD;

void bspstart(void *mbp) {
	Boot::start(mbp);
}

uintptr_t smpstart(uintptr_t *usp) {
	ELF::StartupInfo info;
	size_t total = SMP::getCPUCount();
	/* the running thread has been stored on a different stack last time */
	Thread::setRunning(Thread::getById(0));

	/* start an idle-thread for each cpu */
	for(size_t i = 0; i < total; i++)
		Proc::startThread((uintptr_t)&idlestart,T_IDLE,NULL);

	/* start all APs */
	SMP::start();
	Timer::start(true);

	// remove first page-directory entry. now that all CPUs are started, we don't need that anymore
	(&proc0TLPD)[0] = 0;
	for(int i = 0; i < 3; ++i)
		PageTables::flushAddr(0x200000 * i,true);

	/* start the swapper-thread. it will never return */
	if(PhysMem::canSwap())
		Proc::startThread((uintptr_t)&PhysMem::swapper,0,NULL);
	/* start the terminator */
	Proc::startThread((uintptr_t)&Terminator::start,0,NULL);

	/* load initloader */
	if(ELF::load("/sys/boot/initloader",&info) < 0)
		Util::panic("Unable to load initloader");

	/* give the process some stack pages */
	Thread *t = Thread::getRunning();
	if(!t->reserveFrames(INITIAL_STACK_PAGES))
		Util::panic("Not enough mem for initloader-stack");
	*usp = t->addInitialStack() + INITIAL_STACK_PAGES * PAGE_SIZE - WORDSIZE * 3;
	t->discardFrames();

	extern ulong kstackPtr;
	kstackPtr = KSTACK_AREA + PAGE_SIZE - sizeof(ulong);
	return info.progEntry;
}

void apstart() {
	// TODO
}

static void idlestart() {
	if(!SMP::isBSP()) {
		/* unlock the temporary kernel-stack, so that other CPUs can use it */
		aplock.up();
	}
	thread_idle();
}
