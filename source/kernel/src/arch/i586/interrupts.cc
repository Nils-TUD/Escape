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
#include <sys/arch/i586/task/vm86.h>
#include <sys/arch/i586/task/ioports.h>
#include <sys/arch/i586/pic.h>
#include <sys/arch/i586/lapic.h>
#include <sys/arch/i586/ioapic.h>
#include <sys/dbg/kb.h>
#include <sys/dbg/console.h>
#include <sys/mem/cache.h>
#include <sys/mem/virtmem.h>
#include <sys/task/signals.h>
#include <sys/task/smp.h>
#include <sys/task/thread.h>
#include <sys/task/uenv.h>
#include <sys/task/timer.h>
#include <sys/syscalls.h>
#include <sys/cpu.h>
#include <sys/interrupts.h>
#include <sys/util.h>
#include <sys/video.h>
#include <sys/config.h>
#include <esc/keycodes.h>
#include <esc/syscalls.h>

#define DEBUG_PAGEFAULTS		0

/* maximum number of a exception in a row */
#define MAX_EX_COUNT			10

InterruptsBase::Interrupt InterruptsBase::intrptList[] = {
	/* 0x00 */	{Interrupts::exFatal,		"Divide by zero",		0,					0},
	/* 0x01 */	{Interrupts::exSStep,		"Single step",			0,					0},
	/* 0x02 */	{Interrupts::exFatal,		"Non maskable",			0,					0},
	/* 0x03 */	{Interrupts::exFatal,		"Breakpoint",			0,					0},
	/* 0x04 */	{Interrupts::exFatal,		"Overflow",				0,					0},
	/* 0x05 */	{Interrupts::exFatal,		"Bounds check",			0,					0},
	/* 0x06 */	{Interrupts::exFatal,		"Invalid opcode",		0,					0},
	/* 0x07 */	{Interrupts::exCoProcNA,	"Co-proc. n/a",			0,					0},
	/* 0x08 */	{Interrupts::exFatal,		"Double fault",			0,					0},
	/* 0x09 */	{Interrupts::exFatal,		"Co-proc seg. overrun",	0,					0},
	/* 0x0A */	{Interrupts::exFatal,		"Invalid TSS",			0,					0},
	/* 0x0B */	{Interrupts::exFatal,		"Segment not present",	0,					0},
	/* 0x0C */	{Interrupts::exFatal,		"Stack exception",		0,					0},
	/* 0x0D */	{Interrupts::exGPF,			"Gen. prot. fault",		0,					0},
	/* 0x0E */	{Interrupts::exPF,			"Page fault",			0,					0},
	/* 0x0F */	{NULL,						"??",					0,					0},
	/* 0x10 */	{Interrupts::exFatal,		"Co-processor error",	0,					0},
	/* 0x11 */	{NULL,						"??",					0,					0},
	/* 0x12 */	{NULL,						"??",					0,					0},
	/* 0x13 */	{NULL,						"??",					0,					0},
	/* 0x14 */	{NULL,						"??",					0,					0},
	/* 0x15 */	{NULL,						"??",					0,					0},
	/* 0x16 */	{NULL,						"??",					0,					0},
	/* 0x17 */	{NULL,						"??",					0,					0},
	/* 0x18 */	{NULL,						"??",					0,					0},
	/* 0x19 */	{NULL,						"??",					0,					0},
	/* 0x1A */	{NULL,						"??",					0,					0},
	/* 0x1B */	{NULL,						"??",					0,					0},
	/* 0x1C */	{NULL,						"??",					0,					0},
	/* 0x1D */	{NULL,						"??",					0,					0},
	/* 0x1E */	{NULL,						"??",					0,					0},
	/* 0x1F */	{NULL,						"??",					0,					0},
	/* 0x20 */	{Interrupts::irqTimer,		"PIT",					SIG_INTRPT_TIMER,	0},
	/* 0x21 */	{Interrupts::irqDefault,	"Keyboard",				SIG_INTRPT_KB,		0},
	/* 0x22 */	{NULL,						"??",					0,					0},
	/* 0x23 */	{Interrupts::irqDefault,	"COM2",					SIG_INTRPT_COM2,	0},
	/* 0x24 */	{Interrupts::irqDefault,	"COM1",					SIG_INTRPT_COM1,	0},
	/* 0x25 */	{NULL,						"??",					0,					0},
	/* 0x26 */	{Interrupts::irqDefault,	"Floppy",				SIG_INTRPT_FLOPPY,	0},
	/* 0x27 */	{NULL,						"??",					0,					0},
	/* 0x28 */	{Interrupts::irqDefault,	"CMOS",					SIG_INTRPT_CMOS,	0},
	/* 0x29 */	{NULL,						"??",					0,					0},
	/* 0x2A */	{NULL,						"??",					0,					0},
	/* 0x2B */	{NULL,						"??",					0,					0},
	/* 0x2C */	{Interrupts::irqDefault,	"Mouse",				SIG_INTRPT_MOUSE,	0},
	/* 0x2D */	{NULL,						"??",					0,					0},
	/* 0x2E */	{Interrupts::irqDefault,	"ATA1",					SIG_INTRPT_ATA1,	0},
	/* 0x2F */	{Interrupts::irqDefault,	"ATA2",					SIG_INTRPT_ATA2,	0},
	/* 0x30 */	{Interrupts::debug,			"Debug-Call",			0,					0},
	/* 0x31 */	{Syscalls::handle,			"Ack-Signal",			0,					0},
	/* 0x32 */	{Interrupts::irqTimer,		"LAPIC",				SIG_INTRPT_TIMER,	0},
	/* 0x33 */	{Interrupts::ipiWork,		"Work IPI",				0,					0},
	/* 0x34 */	{Interrupts::ipiTerm,		"Term IPI",				0,					0},
	/* 0x35 */	{NULL,						"??",					0,					0},	// Flush TLB
	/* 0x36 */	{NULL,						"??",					0,					0},	// Wait
	/* 0x37 */	{NULL,						"??",					0,					0},	// Halt
	/* 0x38 */	{NULL,						"??",					0,					0},	// Flush TLB-Ack
	/* 0x39 */	{Interrupts::ipiCallback,	"IPI Callback",			0,					0},
	/* 0x3A */	{Interrupts::exFatal,		"??",					0,					0},
};

uintptr_t *Interrupts::pfAddrs;
size_t Interrupts::exCount = 0;
uint32_t Interrupts::lastEx = 0xFFFFFFFF;
#if DEBUG_PAGEFAULTS
static uintptr_t lastPFAddr = ~(uintptr_t)0;
static pid_t lastPFProc = INVALID_PID;
#endif

void InterruptsBase::init() {
	static_assert(IRQ_COUNT == ARRAY_SIZE(intrptList),"IRQ_COUNT is wrong");
	Interrupts::pfAddrs = (uintptr_t*)Cache::alloc(SMP::getCPUCount() * sizeof(uintptr_t));
	if(Interrupts::pfAddrs == NULL)
		Util::panic("Unable to alloc memory for pagefault-addresses");

	PIC::init();
	if(IOAPIC::enabled()) {
		Log::get().writef("Using IOAPIC for interrupts\n");
		/* identity map ISA irqs that have no interrupt source override */
		uint isaIRQs[] = {0,1,3,4,6,8,12,14,15};
		for(size_t i = 0; i < ARRAY_SIZE(isaIRQs); ++i) {
			if(isaIRQs[i] == 0 && !Config::get(Config::FORCE_PIT))
				continue;

			IOAPIC::setRedirection(isaIRQs[i],isaIRQs[i],IOAPIC::RED_DEL_FIXED,
				IOAPIC::RED_POL_HIGH_ACTIVE,IOAPIC::RED_TRIGGER_EDGE);
		}
		PIC::disable();
	}
	else
		Log::get().writef("Using PIC for interrupts\n");
}

void Interrupts::eoi(int irq) {
	if(irq == IRQ_LAPIC || IOAPIC::enabled())
		LAPIC::eoi();
	else
		PIC::eoi(irq);
}

void Interrupts::syscall(IntrptStackFrame *stack) {
	Thread *t = Thread::getRunning();
	t->pushIntrptLevel(stack);
	t->getStats().syscalls++;
	Syscalls::handle(t,stack);

	/* handle signal */
	t = Thread::getRunning();
	if(t->hasSignalQuick())
		UEnv::handleSignal(t,stack);
	t->popIntrptLevel();
}

void InterruptsBase::handler(IntrptStackFrame *stack) {
	Thread *t = Thread::getRunning();
	Interrupts::Interrupt *intrpt;
	size_t level = t->pushIntrptLevel(stack);

	/* we need to save the page-fault address here because swapping may cause other ones */
	if(stack->intrptNo == Interrupts::EX_PAGE_FAULT)
		Interrupts::pfAddrs[t->getCPU()] = CPU::getCR2();

	intrpt = Interrupts::intrptList + stack->intrptNo;
	intrpt->count++;
	if(intrpt->handler)
		intrpt->handler(t,stack);
	else {
		Log::get().writef("Got interrupt %d (%s) @ 0x%x in process %d (%s)\n",stack->intrptNo,
				intrpt->name,stack->getIP(),t->getProc()->getPid(),t->getProc()->getProgram());
		Interrupts::eoi(stack->intrptNo);
	}

	/* handle signal */
	t = Thread::getRunning();
	if(level == 1) {
		if(t->haveHigherPrio())
			Thread::switchAway();
		if(t->hasSignalQuick())
			UEnv::handleSignal(t,stack);
	}

	t->popIntrptLevel();
}

void Interrupts::debug(A_UNUSED Thread *t,A_UNUSED IntrptStackFrame *stack) {
	Console::start(NULL);
}

void Interrupts::exFatal(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	Log::get().writef("Got exception %x @ %p, process %d:%s\n",stack->intrptNo,stack->getIP(),
			t->getProc()->getPid(),t->getProc()->getProgram());
	/* count consecutive occurrences */
	if(lastEx == stack->intrptNo) {
		exCount++;

		/* stop here? */
		if(exCount >= MAX_EX_COUNT) {
			Util::panic("Got this exception (0x%x) %d times. Stopping here (@ 0x%x)\n",
					stack->intrptNo,exCount,stack->getIP());
		}
	}
	else {
		exCount = 0;
		lastEx = stack->intrptNo;
	}
}

void Interrupts::exGPF(Thread *t,IntrptStackFrame *stack) {
	/* io-map not loaded yet? */
	if(IOPorts::handleGPF()) {
		exCount = 0;
		return;
	}
	/* vm86-task? */
	if(t->getProc()->getFlags() & P_VM86) {
		VM86::handleGPF(static_cast<VM86IntrptStackFrame*>(stack));
		exCount = 0;
		return;
	}
	/* TODO later the process should be killed here */
	Util::panic("GPF @ 0x%x",stack->getIP());
}

void Interrupts::exSStep(A_UNUSED Thread *t,A_UNUSED IntrptStackFrame *stack) {
	Util::switchToVGA();
	Console::start("b i;step show");
}

void Interrupts::exCoProcNA(Thread *t,A_UNUSED IntrptStackFrame *stack) {
	FPU::handleCoProcNA(t->getFPUState());
}

void Interrupts::exPF(Thread *t,IntrptStackFrame *stack) {
	uintptr_t addr = pfAddrs[t->getCPU()];

#if DEBUG_PAGEFAULTS
	if(addr == lastPFAddr && lastPFProc == Proc::getRunning()) {
		exCount++;
		if(exCount >= MAX_EX_COUNT)
			Util::panic("%d page-faults at the same address of the same process",exCount);
	}
	else
		exCount = 0;
	lastPFAddr = addr;
	lastPFProc = Proc::getRunning();
	printPFInfo(Log::get(),t,stack,addr);
#endif

	/* first let the vmm try to handle the page-fault (demand-loading, cow, swapping, ...) */
	int res = VirtMem::pagefault(addr,stack->getError() & 0x2);
	if(EXPECT_TRUE(res == 0))
		return;
	/* ok, now lets check if the thread wants more stack-pages */
	if(EXPECT_TRUE(res == -EFAULT && (res = Thread::extendStack(addr)) == 0))
		return;

	printPFInfo(Log::get(),t,stack,addr);
	Log::get().writef("Unable to resolve because: %s (%d)\n",strerror(-res),res);
#if PANIC_ON_PAGEFAULT
	Util::setpf(addr,stack->getIP());
	Util::panic("Process segfaulted");
#else
	Proc::segFault();
#endif
}

void Interrupts::irqTimer(Thread *t,IntrptStackFrame *stack) {
	assert(t->getIntrptLevel() == 0);
	bool res = false;
	const Interrupt *intrpt = intrptList + stack->intrptNo;
	if(intrpt->signal && t->getCPU() == 0) {
		/* the idle-task HAS TO switch to another thread if he given somebody a signal. otherwise
		 * we would wait for the next thread-switch (e.g. caused by a timer-irq). */
		if(Signals::addSignal(intrpt->signal) && (t->getFlags() & T_IDLE))
			res = true;
	}
	res |= Timer::intrpt();
	eoi(stack->intrptNo);
	if(res)
		Thread::switchAway();
}

void Interrupts::irqDefault(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	assert(t->getIntrptLevel() == 0);
	bool res = false;
	const Interrupt *intrpt = intrptList + stack->intrptNo;
	if(intrpt->signal) {
		if(Signals::addSignal(intrpt->signal) && (t->getFlags() & T_IDLE))
			res = true;
	}
	eoi(stack->intrptNo);

	/* in debug-mode, start the logviewer when the keyboard is not present yet */
	/* (with a present keyboard-device we would steal him the scancodes) */
	/* this way, we can debug the system in the startup-phase without affecting timings
	 * (before viewing the log ;)) */
	if(stack->intrptNo == IRQ_KEYBOARD && Proc::getByPid(KEYBOARD_PID) == NULL) {
		Keyboard::Event ev;
		if(Keyboard::get(&ev,KEV_PRESS,false) && ev.keycode == VK_F12)
			Console::start(NULL);
	}
	if(res)
		Thread::switchAway();
}

void Interrupts::ipiWork(Thread *t,A_UNUSED IntrptStackFrame *stack) {
	LAPIC::eoi();
	/* if we have been waked up, but are not idling anymore, wakeup another cpu */
	/* this may happen if we're about to switch to a non-idle-thread and have idled previously,
	 * while SMP::wakeupCPU() was called. */
	if(!(t->getFlags() & T_IDLE))
		SMP::wakeupCPU();
	/* otherwise switch to non-idle-thread (if there is any) */
	else
		Thread::switchAway();
}

void Interrupts::ipiTerm(Thread *t,A_UNUSED IntrptStackFrame *stack) {
	/* stop running the thread, if it should die */
	if(t->getNewState() == Thread::ZOMBIE)
		Thread::switchAway();
}

void Interrupts::ipiCallback(Thread *t,A_UNUSED IntrptStackFrame *stack) {
	SMP::callback(t->getCPU());
	LAPIC::eoi();
}

void Interrupts::printPFInfo(OStream &os,Thread *t,IntrptStackFrame *stack,uintptr_t addr) {
	os.writef("Page fault for address %p @ %p on CPU %d, process %d\n",
			addr,stack->getIP(),t->getCPU(),t->getProc()->getPid());
	os.writef("Occurred because:\n\t%s\n\t%s\n\t%s\n\t%s%s\n",
			(stack->getError() & 0x1) ?
				"page-level protection violation" : "not-present page",
			(stack->getError() & 0x2) ? "write" : "read",
			(stack->getError() & 0x4) ? "user-mode" : "kernel-mode",
			(stack->getError() & 0x8) ? "reserved bits set to 1\n\t" : "",
			(stack->getError() & 0x16) ? "instruction-fetch" : "");
}

void InterruptsBase::printStackFrame(OStream &os,const IntrptStackFrame *stack) {
	os.writef("stack-frame @ 0x%x\n",stack);
	os.writef("\teax=0x%08x\n",stack->eax);
	os.writef("\tebx=0x%08x\n",stack->ebx);
	os.writef("\tecx=0x%08x\n",stack->ecx);
	os.writef("\tedx=0x%08x\n",stack->edx);
	os.writef("\tesi=0x%08x\n",stack->esi);
	os.writef("\tedi=0x%08x\n",stack->edi);
	os.writef("\tebp=0x%08x\n",stack->ebp);
	os.writef("\tuesp=0x%08x\n",stack->getSP());
	os.writef("\teip=0x%08x\n",stack->getIP());
	os.writef("\teflags=0x%08x\n",stack->getFlags());
	if(stack->intrptNo) {
		os.writef("\terrorCode=%d\n",stack->getError());
		os.writef("\tintrptNo=%d\n",stack->intrptNo);
	}
}
