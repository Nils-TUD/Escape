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
#include <sys/task/signals.h>
#include <sys/task/timer.h>
#include <sys/task/uenv.h>
#include <sys/task/proc.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/virtmem.h>
#include <sys/dbg/kb.h>
#include <sys/dbg/console.h>
#include <esc/keycodes.h>
#include <sys/cpu.h>
#include <sys/syscalls.h>
#include <sys/interrupts.h>
#include <sys/video.h>
#include <sys/util.h>

#define DEBUG_PAGEFAULTS	0

const Interrupts::Interrupt Interrupts::intrptList[] = {
	/* 0x00: IRQ_TTY0_XMTR */	{defHandler,	"Terminal 0 Transmitter",0},
	/* 0x01: IRQ_TTY0_RCVR */	{defHandler,	"Terminal 0 Receiver",	0},
	/* 0x02: IRQ_TTY1_XMTR */	{defHandler,	"Terminal 1 Transmitter",0},
	/* 0x03: IRQ_TTY1_RCVR */	{defHandler,	"Terminal 1 Receiver",	0},
	/* 0x04: IRQ_KEYBOARD */	{irqKB,			"Keyboard",				SIG_INTRPT_KB},
	/* 0x05: -- */				{defHandler,	"??",					0},
	/* 0x06: -- */				{defHandler,	"??",					0},
	/* 0x07: -- */				{defHandler,	"??",					0},
	/* 0x08: IRQ_DISK */		{irqDisk,		"Disk",					SIG_INTRPT_ATA1},
	/* 0x09: -- */				{defHandler,	"??",					0},
	/* 0x0A: - */				{defHandler,	"??",					0},
	/* 0x0B: -- */				{defHandler,	"??",					0},
	/* 0x0C: -- */				{defHandler,	"??",					0},
	/* 0x0D: -- */				{defHandler,	"??",					0},
	/* 0x0E: IRQ_TIMER */		{irqTimer,		"Timer",				SIG_INTRPT_TIMER},
	/* 0x0F: -- */				{defHandler,	"??",					0},
	/* 0x10: EXC_BUS_TIMEOUT */	{defHandler,	"Bus timeout exception",0},
	/* 0x11: EXC_ILL_INSTRCT */	{defHandler,	"Ill. instr. exception",0},
	/* 0x12: EXC_PRV_INSTRCT */	{defHandler,	"Prv. instr. exception",0},
	/* 0x13: EXC_DIVIDE */		{defHandler,	"Divide exception",		0},
	/* 0x14: EXC_TRAP */		{exTrap,		"Trap exception",		0},
	/* 0x15: EXC_TLB_MISS */	{defHandler,	"TLB miss exception",	0},
	/* 0x16: EXC_TLB_WRITE */	{exPageFault,	"TLB write exception",	0},
	/* 0x17: EXC_TLB_INVALID */	{exPageFault,	"TLB invalid exception",0},
	/* 0x18: EXC_ILL_ADDRESS */	{defHandler,	"Ill. addr. exception",	0},
	/* 0x19: EXC_PRV_ADDRESS */	{defHandler,	"Prv. addr. exception",	0},
	/* 0x1A: -- */				{defHandler,	"??",					0},
	/* 0x1B: -- */				{defHandler,	"??",					0},
	/* 0x1C: -- */				{defHandler,	"??",					0},
	/* 0x1D: -- */				{defHandler,	"??",					0},
	/* 0x1E: -- */				{defHandler,	"??",					0},
	/* 0x1F: -- */				{defHandler,	"??",					0},
};
size_t Interrupts::irqCount = 0;
uintptr_t Interrupts::pfaddr = 0;

void InterruptsBase::handler(IntrptStackFrame *stack) {
	const Interrupts::Interrupt *intrpt;
	/* note: we have to do that before there is a chance for a kernel-miss */
	Interrupts::pfaddr = CPU::getBadAddr();
	Thread *t = Thread::getRunning();
	t->pushIntrptLevel(stack);
	Interrupts::irqCount++;

	/* call handler */
	intrpt = Interrupts::intrptList + (stack->irqNo & 0x1F);
	intrpt->handler(stack);

	/* only handle signals, if we come directly from user-mode */
	/* note: we might get a kernel-miss at arbitrary places in the kernel; if we checked for
	 * signals in that case, we might cause a thread-switch. this is not always possible! */
	t = Thread::getRunning();
	if((t->getFlags() & T_IDLE) || (stack->psw & Interrupts::PSW_PUM)) {
		if(t->haveHigherPrio())
			Thread::switchAway();
		if(t->hasSignalQuick())
			UEnv::handleSignal(t,stack);
	}

	t->popIntrptLevel();
}

void Interrupts::defHandler(IntrptStackFrame *stack) {
	/* do nothing */
	Util::panic("Got interrupt %d (%s) @ %p\n",
			stack->irqNo,intrptList[stack->irqNo & 0x1f].name,stack->r[30]);
}

void Interrupts::exTrap(IntrptStackFrame *stack) {
	Thread *t = Thread::getRunning();
	t->getStats().syscalls++;
	Syscalls::handle(t,stack);
	/* skip trap-instruction */
	stack->r[30] += 4;
}

void Interrupts::debug(A_UNUSED IntrptStackFrame *stack) {
	Console::start(NULL);
}

void Interrupts::exPageFault(IntrptStackFrame *stack) {
#if DEBUG_PAGEFAULTS
	if(pfaddr == lastPFAddr && lastPFProc == Proc::getRunning()->getPid()) {
		exCount++;
		if(exCount >= MAX_EX_COUNT)
			Util::panic("%d page-faults at the same address of the same process",exCount);
	}
	else
		exCount = 0;
	lastPFAddr = pfaddr;
	lastPFProc = Proc::getRunning()->getPid();
	os.writef("Page fault for address=0x%08x @ 0x%x, process %d\n",pfaddr,
			stack->r[30],Proc::getRunning()->getPid());
#endif

	/* first let the vmm try to handle the page-fault (demand-loading, cow, swapping, ...) */
	int res = VirtMem::pagefault(pfaddr,stack->irqNo == EXC_TLB_WRITE);
	if(EXPECT_TRUE(res == 0))
		return;
	/* ok, now lets check if the thread wants more stack-pages */
	if(EXPECT_TRUE(res == -EFAULT && (res = Thread::extendStack(pfaddr)) == 0))
		return;

	pid_t pid = Proc::getRunning();
	Log::get().writef("proc %d, page fault for address %p @ %p\n",pid,pfaddr,stack->r[30]);
	Log::get().writef("Unable to resolve because: %s (%d)\n",strerror(-res),res);
#if PANIC_ON_PAGEFAULT
	Util::setpf(pfaddr,stack->r[30]);
	Util::panic("proc %d: page fault for address %p @ %p\n",pid,pfaddr,stack->r[30]);
#else
	Proc::segFault();
#endif
}

void Interrupts::irqTimer(A_UNUSED IntrptStackFrame *stack) {
	bool res = Signals::addSignal(SIG_INTRPT_TIMER);
	res |= Timer::intrpt();
	Timer::ackIntrpt();
	if(res) {
		Thread *t = Thread::getRunning();
		if(t->getIntrptLevel() == 0)
			Thread::switchAway();
	}
}

void Interrupts::irqKB(A_UNUSED IntrptStackFrame *stack) {
	/* we have to disable interrupts until the device has handled the request */
	/* otherwise we would get into an interrupt loop */
	uint32_t *kbRegs = (uint32_t*)KEYBOARD_BASE;
	kbRegs[KEYBOARD_CTRL] &= ~KEYBOARD_IEN;

	if(Proc::getByPid(KEYBOARD_PID) == NULL) {
		/* in debug-mode, start the logviewer when the keyboard is not present yet */
		/* (with a present keyboard-device we would steal him the scancodes) */
		/* this way, we can debug the system in the startup-phase without affecting timings
		 * (before viewing the log ;)) */
		Keyboard::Event ev;
		if(Keyboard::get(&ev,KEV_PRESS,false) && ev.keycode == VK_F12)
			Console::start(NULL);
	}

	/* we can't add the signal before the kb-interrupts are disabled; otherwise a kernel-miss might
	 * call UEnv::handleSignal(), which might cause a thread-switch */
	if(!Signals::addSignal(SIG_INTRPT_KB)) {
		/* if there is no device that handles the signal, reenable interrupts */
		kbRegs[KEYBOARD_CTRL] |= KEYBOARD_IEN;
	}
	else
		Thread::switchAway();
}

void Interrupts::irqDisk(A_UNUSED IntrptStackFrame *stack) {
	/* see Interrupts::irqKb() */
	uint32_t *diskRegs = (uint32_t*)DISK_BASE;
	diskRegs[DISK_CTRL] &= ~DISK_IEN;
	if(!Signals::addSignal(SIG_INTRPT_ATA1))
		diskRegs[DISK_CTRL] |= DISK_IEN;
	else
		Thread::switchAway();
}

void InterruptsBase::printStackFrame(OStream &os,const IntrptStackFrame *stack) {
	os.writef("stack-frame @ 0x%Px\n",stack);
	os.writef("\tirqNo=%d\n",stack->irqNo);
	os.writef("\tpsw=#%08x\n",stack->psw);
	os.writef("\tregister:\n");
	for(int i = 0; i < REG_COUNT; i++)
		os.writef("\tr[%d]=#%08x\n",i,stack->r[i]);
}
