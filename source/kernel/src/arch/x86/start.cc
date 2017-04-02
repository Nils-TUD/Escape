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

#include <arch/x86/fpu.h>
#include <arch/x86/gdt.h>
#include <arch/x86/idt.h>
#include <arch/x86/lapic.h>
#include <mem/pagedir.h>
#include <mem/virtmem.h>
#include <task/elf.h>
#include <task/proc.h>
#include <task/smp.h>
#include <task/terminator.h>
#include <task/thread.h>
#include <assert.h>
#include <boot.h>
#include <common.h>
#include <cpu.h>
#include <spinlock.h>
#include <util.h>
#include <video.h>

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
	/* the running thread has been stored on a different stack last time */
	Thread *t = Thread::getById(0);
	Thread::setRunning(t);

	/* start an idle-thread for each cpu */
	size_t total = SMP::getCPUCount();
	for(size_t i = 1; i < total; i++)
		Proc::startThread((uintptr_t)&idlestart,T_IDLE,NULL);

	/* start all APs */
	SMP::start();
	Timer::start(true);

	/* remove initial PTs. since all CPUs are started, we don't need that anymore */
	(&proc0TLPD)[0] = 0;
	for(int i = 0; i < PT_LEVELS - 1; ++i)
		PageTables::flushAddr(PT_SIZE * i,true);

	/* start init process */
	if(Proc::clone(P_KERNEL) == 0) {
		Thread *t = Thread::getRunning();
		pid_t pid = t->getProc()->getPid();

		/* load initloader */
		OpenFile *file;
		if(VFS::openPath(pid,VFS_EXEC | VFS_READ,0,"/sys/boot/initloader",&file) < 0)
			Util::panic("Unable to open initloader");
		if(ELF::load(file,&info) < 0)
			Util::panic("Unable to load initloader");
		file->close(pid);

		/* give the process some stack pages */
		if(!t->reserveFrames(INITIAL_STACK_PAGES))
			Util::panic("Not enough mem for initloader-stack");
		*usp = t->addInitialStack() + INITIAL_STACK_PAGES * PAGE_SIZE - WORDSIZE * 3;
		t->discardFrames();
		return info.progEntry;
	}

	/* start the swapper-process. it will never return */
	if(PhysMem::canSwap())
		Proc::startKProc("[swapper]",&PhysMem::swapper);
	/* and the terminator */
	Proc::startKProc("[terminator]",&Terminator::start);

	thread_idle();
	A_UNREACHED;
}

void apstart() {
	/* before we do anything, enable NXE if necessary. otherwise we can't use the pagetables */
	PageDir::enableNXE();
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
	/* unlock the temporary kernel-stack, so that other CPUs can use it */
	aplock.up();
	thread_idle();
}
