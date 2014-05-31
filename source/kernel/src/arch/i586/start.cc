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

#include <sys/common.h>
#include <sys/arch/x86/idt.h>
#include <sys/arch/x86/gdt.h>
#include <sys/arch/x86/fpu.h>
#include <sys/arch/x86/lapic.h>
#include <sys/arch/i586/task/vm86.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/elf.h>
#include <sys/task/smp.h>
#include <sys/task/terminator.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/pagedir.h>
#include <sys/cpu.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <assert.h>

/* make gcc happy */
EXTERN_C void bspstart(void *mbp);
EXTERN_C uintptr_t smpstart(uintptr_t *usp);
EXTERN_C void apstart();
static void idlestart();
extern SpinLock aplock;
extern ulong proc0TLPD;

void bspstart(void *mbp) {
	/* init the kernel */
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
	PageTables::flushAddr(0,true);

	/* start the swapper-thread. it will never return */
	if(PhysMem::canSwap())
		Proc::startThread((uintptr_t)&PhysMem::swapper,0,NULL);
	/* start the terminator */
	Proc::startThread((uintptr_t)&Terminator::start,0,NULL);
	if(VM86::create() < 0)
		Util::panic("Unable to create VM86 task");

	/* load initloader */
	if(ELF::load("/system/boot/initloader",&info) < 0)
		Util::panic("Unable to load initloader");

	/* give the process some stack pages */
	Thread *t = Thread::getRunning();
	if(!t->reserveFrames(INITIAL_STACK_PAGES))
		Util::panic("Not enough mem for initloader-stack");
	*usp = t->addInitialStack() + INITIAL_STACK_PAGES * PAGE_SIZE - WORDSIZE;
	t->discardFrames();
	return info.progEntry;
}

void apstart() {
	/* store the running thread for our temp-stack again, because we might need it in gdt_init_ap
	 * for example */
	Thread::setRunning(Thread::getById(0));
	/* at first, setup our GDT properly */
	GDT::initAP();
	/* setup IDT for this cpu and enable its local APIC */
	IDT::init();
	LAPIC::enable();
	Timer::start(false);
	/* init FPU and detect our CPU */
	FPU::init();
	CPU::detect();
	/* notify the BSP that we're running */
	SMP::apIsRunning();
	/* choose a thread to run */
	Thread::initialSwitch();
}

static void idlestart() {
	if(!SMP::isBSP()) {
		/* unlock the temporary kernel-stack, so that other CPUs can use it */
		aplock.up();
	}
	thread_idle();
}
