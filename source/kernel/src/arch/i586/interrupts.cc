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
#include <esc/keycodes.h>
#include <esc/syscalls.h>

#define DEBUG_PAGEFAULTS		0

/* maximum number of a exception in a row */
#define MAX_EX_COUNT			10

const Interrupts::Interrupt Interrupts::intrptList[] = {
	/* 0x00: EX_DIVIDE_BY_ZERO */	{exFatal,		"Divide by zero",		0},
	/* 0x01: EX_SINGLE_STEP */		{exSStep,		"Single step",			0},
	/* 0x02: EX_NONMASKABLE */		{exFatal,		"Non maskable",			0},
	/* 0x03: EX_BREAKPOINT */		{exFatal,		"Breakpoint",			0},
	/* 0x04: EX_OVERFLOW */			{exFatal,		"Overflow",				0},
	/* 0x05: EX_BOUNDS_CHECK */		{exFatal,		"Bounds check",			0},
	/* 0x06: EX_INVALID_OPCODE */	{exFatal,		"Invalid opcode",		0},
	/* 0x07: EX_CO_PROC_NA */		{exCoProcNA,	"Co-proc. n/a",			0},
	/* 0x08: EX_DOUBLE_FAULT */		{exFatal,		"Double fault",			0},
	/* 0x09: EX_CO_PROC_SEG_OVR */	{exFatal,		"Co-proc seg. overrun",	0},
	/* 0x0A: EX_INVALID_TSS */		{exFatal,		"Invalid TSS",			0},
	/* 0x0B: EX_SEG_NOT_PRESENT */	{exFatal,		"Segment not present",	0},
	/* 0x0C: EX_STACK */			{exFatal,		"Stack exception",		0},
	/* 0x0D: EX_GEN_PROT_FAULT */	{exGPF,			"Gen. prot. fault",		0},
	/* 0x0E: EX_PAGE_FAULT */		{exPF,			"Page fault",			0},
	/* 0x0F: -- */					{NULL,			"Unknown",				0},
	/* 0x10: EX_CO_PROC_ERROR */	{exFatal,		"Co-processor error",	0},
	/* 0x11: -- */					{NULL,			"Unknown",				0},
	/* 0x12: -- */					{NULL,			"Unknown",				0},
	/* 0x13: -- */					{NULL,			"Unknown",				0},
	/* 0x14: -- */					{NULL,			"Unknown",				0},
	/* 0x15: -- */					{NULL,			"Unknown",				0},
	/* 0x16: -- */					{NULL,			"Unknown",				0},
	/* 0x17: -- */					{NULL,			"Unknown",				0},
	/* 0x18: -- */					{NULL,			"Unknown",				0},
	/* 0x19: -- */					{NULL,			"Unknown",				0},
	/* 0x1A: -- */					{NULL,			"Unknown",				0},
	/* 0x1B: -- */					{NULL,			"Unknown",				0},
	/* 0x1C: -- */					{NULL,			"Unknown",				0},
	/* 0x1D: -- */					{NULL,			"Unknown",				0},
	/* 0x1E: -- */					{NULL,			"Unknown",				0},
	/* 0x1F: -- */					{NULL,			"Unknown",				0},
	/* 0x20: IRQ_TIMER */			{irqTimer,		"Timer",				SIG_INTRPT_TIMER},
	/* 0x21: IRQ_KEYBOARD */		{irqDefault,	"Keyboard",				SIG_INTRPT_KB},
	/* 0x22: -- */					{NULL,			"Unknown",				0},
	/* 0x23: IRQ_COM2 */			{irqDefault,	"COM2",					SIG_INTRPT_COM2},
	/* 0x24: IRQ_COM1 */			{irqDefault,	"COM1",					SIG_INTRPT_COM1},
	/* 0x25: -- */					{NULL,			"Unknown",				0},
	/* 0x26: IRQ_FLOPPY */			{irqDefault,	"Floppy",				SIG_INTRPT_FLOPPY},
	/* 0x27: -- */					{NULL,			"Unknown",				0},
	/* 0x28: IRQ_CMOS_RTC */		{irqDefault,	"CMOS",					SIG_INTRPT_CMOS},
	/* 0x29: -- */					{NULL,			"Unknown",				0},
	/* 0x2A: -- */					{NULL,			"Unknown",				0},
	/* 0x2B: -- */					{NULL,			"Unknown",				0},
	/* 0x2C: IRQ_MOUSE */			{irqDefault,	"Mouse",				SIG_INTRPT_MOUSE},
	/* 0x2D: -- */					{NULL,			"Unknown",				0},
	/* 0x2C: IRQ_ATA1 */			{irqDefault,	"ATA1",					SIG_INTRPT_ATA1},
	/* 0x2C: IRQ_ATA2 */			{irqDefault,	"ATA2",					SIG_INTRPT_ATA2},
	/* 0x30: syscall */				{syscall,		"Systemcall",			0},
	/* 0x31: IPI_WORK */			{ipiWork,		"Work IPI",				0},
	/* 0x32: IPI_TERM */			{ipiTerm,		"Term IPI",				0},
	/* 0x33: IPI_FLUSH_TLB */		{exFatal,		"Flush TLB IPI",		0},	/* not handled here */
	/* 0x34: IPI_WAIT */			{exFatal,		"",						0},
	/* 0x35: IPI_HALT */			{exFatal,		"",						0},
	/* 0x36: IPI_FLUSH_TLB */		{exFatal,		"",						0},
	/* 0x37: isrNull */				{exFatal,		"",						0},
};

size_t Interrupts::intrptCount = 0;
uintptr_t *Interrupts::pfAddrs;
size_t Interrupts::exCount = 0;
uint32_t Interrupts::lastEx = 0xFFFFFFFF;
#if DEBUG_PAGEFAULTS
static uintptr_t lastPFAddr = ~(uintptr_t)0;
static pid_t lastPFProc = INVALID_PID;
#endif

void InterruptsBase::init() {
	Interrupts::pfAddrs = (uintptr_t*)Cache::alloc(SMP::getCPUCount() * sizeof(uintptr_t));
	if(Interrupts::pfAddrs == NULL)
		Util::panic("Unable to alloc memory for pagefault-addresses");
}

void InterruptsBase::handler(IntrptStackFrame *stack) {
	Thread *t = Thread::getRunning();
	const Interrupts::Interrupt *intrpt;
	size_t level = t->pushIntrptLevel(stack);
	Interrupts::intrptCount++;

	/* we need to save the page-fault address here because swapping may cause other ones */
	if(stack->intrptNo == Interrupts::EX_PAGE_FAULT)
		Interrupts::pfAddrs[t->getCPU()] = CPU::getCR2();

	intrpt = Interrupts::intrptList + stack->intrptNo;
	if(intrpt->handler)
		intrpt->handler(t,stack);
	else {
		vid_printf("Got interrupt %d (%s) @ 0x%x in process %d (%s)\n",stack->intrptNo,
				intrpt->name,stack->eip,t->getProc()->getPid(),t->getProc()->getCommand());
	}

	/* handle signal */
	t = Thread::getRunning();
	if(level == 1)
		UEnv::handleSignal(t,stack);

	t->popIntrptLevel();
}

void Interrupts::exFatal(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	vid_printf("Got exception %x @ %p, process %d:%s\n",stack->intrptNo,stack->eip,
			t->getProc()->getPid(),t->getProc()->getCommand());
	/* count consecutive occurrences */
	if(lastEx == stack->intrptNo) {
		exCount++;

		/* stop here? */
		if(exCount >= MAX_EX_COUNT) {
			/* for exceptions in kernel: ensure that we have the default print-function */
			if(stack->eip >= KERNEL_AREA)
				vid_unsetPrintFunc();
			Util::panic("Got this exception (0x%x) %d times. Stopping here (@ 0x%x)\n",
					stack->intrptNo,exCount,stack->eip);
		}
	}
	else {
		exCount = 0;
		lastEx = stack->intrptNo;
	}
}

void Interrupts::exGPF(Thread *t,IntrptStackFrame *stack) {
	/* for exceptions in kernel: ensure that we have the default print-function */
	if(stack->eip >= KERNEL_AREA)
		vid_unsetPrintFunc();
	/* io-map not loaded yet? */
	if(IOPorts::handleGPF()) {
		exCount = 0;
		return;
	}
	/* vm86-task? */
	if(t->getProc()->getFlags() & P_VM86) {
		VM86::handleGPF(stack);
		exCount = 0;
		return;
	}
	/* TODO later the process should be killed here */
	Util::panic("GPF @ 0x%x",stack->eip);
}

void Interrupts::exSStep(A_UNUSED Thread *t,A_UNUSED IntrptStackFrame *stack) {
	Console::start("step show");
}

void Interrupts::exCoProcNA(Thread *t,A_UNUSED IntrptStackFrame *stack) {
	FPU::handleCoProcNA(t->getFPUState());
}

void Interrupts::exPF(Thread *t,IntrptStackFrame *stack) {
	uintptr_t addr = pfAddrs[t->getCPU()];
	/* for exceptions in kernel: ensure that we have the default print-function */
	if(stack->eip >= KERNEL_AREA) {
		vid_setTargets(TARGET_LOG | TARGET_SCREEN);
		vid_unsetPrintFunc();
	}

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
	intrpt_printPFInfo(stack,addr);
#endif

	/* first let the vmm try to handle the page-fault (demand-loading, cow, swapping, ...) */
	if(!VirtMem::pagefault(addr,stack->errorCode & 0x2)) {
		/* ok, now lets check if the thread wants more stack-pages */
		if(Thread::extendStack(addr) < 0) {
			printPFInfo(stack,addr);
			/* TODO Proc::segFault();*/
			Util::panic("Process segfaulted");
		}
	}
}

void Interrupts::irqTimer(Thread *t,IntrptStackFrame *stack) {
	bool res;
	const Interrupt *intrpt = intrptList + stack->intrptNo;
	if(intrpt->signal)
		Signals::addSignal(intrpt->signal);
	res = Timer::intrpt();
	PIC::eoi(stack->intrptNo);
	if(res) {
		if(t->getIntrptLevel() == 0)
			Thread::switchAway();
	}
}

void Interrupts::irqDefault(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	const Interrupt *intrpt = intrptList + stack->intrptNo;
	if(intrpt->signal)
		Signals::addSignal(intrpt->signal);
	PIC::eoi(stack->intrptNo);
	/* in debug-mode, start the logviewer when the keyboard is not present yet */
	/* (with a present keyboard-device we would steal him the scancodes) */
	/* this way, we can debug the system in the startup-phase without affecting timings
	 * (before viewing the log ;)) */
	if(stack->intrptNo == IRQ_KEYBOARD && Proc::getByPid(KEYBOARD_PID) == NULL) {
		Keyboard::Event ev;
		if(Keyboard::get(&ev,KEV_PRESS,false) && ev.keycode == VK_F12)
			Console::start(NULL);
	}
}

void Interrupts::syscall(Thread *t,IntrptStackFrame *stack) {
	t->getStats().syscalls++;
	Syscalls::handle(t,stack);
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

void Interrupts::printPFInfo(IntrptStackFrame *stack,uintptr_t addr) {
	pid_t pid = Proc::getRunning();
	vid_printf("Page fault for address %p @ %p, process %d\n",addr,stack->eip,pid);
	vid_printf("Occurred because:\n\t%s\n\t%s\n\t%s\n\t%s%s\n",
			(stack->errorCode & 0x1) ?
				"page-level protection violation" : "not-present page",
			(stack->errorCode & 0x2) ? "write" : "read",
			(stack->errorCode & 0x4) ? "user-mode" : "kernel-mode",
			(stack->errorCode & 0x8) ? "reserved bits set to 1\n\t" : "",
			(stack->errorCode & 0x16) ? "instruction-fetch" : "");
}

void InterruptsBase::printStackFrame(const IntrptStackFrame *stack) {
	vid_printf("stack-frame @ 0x%x\n",stack);
	vid_printf("\tcs=%02x\n",stack->cs);
	vid_printf("\tds=%02x\n",stack->ds);
	vid_printf("\tes=%02x\n",stack->es);
	vid_printf("\tfs=%02x\n",stack->fs);
	vid_printf("\tgs=%02x\n",stack->gs);
	vid_printf("\teax=0x%08x\n",stack->eax);
	vid_printf("\tebx=0x%08x\n",stack->ebx);
	vid_printf("\tecx=0x%08x\n",stack->ecx);
	vid_printf("\tedx=0x%08x\n",stack->edx);
	vid_printf("\tesi=0x%08x\n",stack->esi);
	vid_printf("\tedi=0x%08x\n",stack->edi);
	vid_printf("\tebp=0x%08x\n",stack->ebp);
	vid_printf("\tuesp=0x%08x\n",stack->uesp);
	vid_printf("\teip=0x%08x\n",stack->eip);
	vid_printf("\teflags=0x%08x\n",stack->eflags);
	vid_printf("\terrorCode=%d\n",stack->errorCode);
	vid_printf("\tintrptNo=%d\n",stack->intrptNo);
}
