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

InterruptsBase::Interrupt InterruptsBase::intrptList[] = {
	/* 0x00 */	{Interrupts::defHandler,	"Terminal 0 Xmtr",			0},
	/* 0x01 */	{Interrupts::defHandler,	"Terminal 0 Rcvr",			0},
	/* 0x02 */	{Interrupts::defHandler,	"Terminal 1 Xmtr",			0},
	/* 0x03 */	{Interrupts::defHandler,	"Terminal 1 Rcvr",			0},
	/* 0x04 */	{Interrupts::irqKB,			"Keyboard",					0},
	/* 0x05 */	{Interrupts::defHandler,	"??",						0},
	/* 0x06 */	{Interrupts::defHandler,	"??",						0},
	/* 0x07 */	{Interrupts::defHandler,	"??",						0},
	/* 0x08 */	{Interrupts::irqDisk,		"Disk",						0},
	/* 0x09 */	{Interrupts::defHandler,	"??",						0},
	/* 0x0A */	{Interrupts::defHandler,	"??",						0},
	/* 0x0B */	{Interrupts::defHandler,	"??",						0},
	/* 0x0C */	{Interrupts::defHandler,	"??",						0},
	/* 0x0D */	{Interrupts::defHandler,	"??",						0},
	/* 0x0E */	{Interrupts::irqTimer,		"Timer",					0},
	/* 0x0F */	{Interrupts::defHandler,	"??",						0},
	/* 0x10 */	{Interrupts::defHandler,	"Bus timeout ex.",			0},
	/* 0x11 */	{Interrupts::defHandler,	"Ill. instr. ex.",			0},
	/* 0x12 */	{Interrupts::defHandler,	"Prv. instr. ex.",			0},
	/* 0x13 */	{Interrupts::defHandler,	"Divide ex.",				0},
	/* 0x14 */	{Interrupts::exTrap,		"Trap ex.",					0},
	/* 0x15 */	{Interrupts::defHandler,	"TLB miss ex.",				0},
	/* 0x16 */	{Interrupts::exPageFault,	"TLB write ex.",			0},
	/* 0x17 */	{Interrupts::exPageFault,	"TLB invalid ex.",			0},
	/* 0x18 */	{Interrupts::defHandler,	"Ill. addr. ex.",			0},
	/* 0x19 */	{Interrupts::defHandler,	"Prv. addr. ex.",			0},
	/* 0x1A */	{Interrupts::defHandler,	"??",						0},
	/* 0x1B */	{Interrupts::defHandler,	"??",						0},
	/* 0x1C */	{Interrupts::defHandler,	"??",						0},
	/* 0x1D */	{Interrupts::defHandler,	"??",						0},
	/* 0x1E */	{Interrupts::defHandler,	"??",						0},
	/* 0x1F */	{Interrupts::defHandler,	"??",						0},
};

uintptr_t Interrupts::pfaddr = 0;

int InterruptsBase::installHandler(int irq,const char *) {
	if(irq < 0 || irq >= 0x10)
		return -EINVAL;

	if(intrptList[irq].handler == Interrupts::defHandler)
		return -EINVAL;

	/* nothing to do. already installed */
	return 0;
}

void InterruptsBase::uninstallHandler(int irq) {
	/* nothing to do */
}

void InterruptsBase::handler(IntrptStackFrame *stack) {
	static_assert(IRQ_COUNT == ARRAY_SIZE(intrptList),"IRQ_COUNT is wrong");
	Interrupts::Interrupt *intrpt;
	/* note: we have to do that before there is a chance for a kernel-miss */
	Interrupts::pfaddr = CPU::getBadAddr();
	Thread *t = Thread::getRunning();
	t->pushIntrptLevel(stack);

	/* call handler */
	intrpt = Interrupts::intrptList + (stack->irqNo & 0x1F);
	intrpt->count++;
	intrpt->handler(t,stack);

	/* only handle signals, if we come directly from user-mode */
	/* note: we might get a kernel-miss at arbitrary places in the kernel; if we checked for
	 * signals in that case, we might cause a thread-switch. this is not always possible! */
	t = Thread::getRunning();
	if((t->getFlags() & T_IDLE) || (stack->psw & Interrupts::PSW_PUM)) {
		if(t->haveHigherPrio())
			Thread::switchAway();
			UEnv::handleSignal(t,stack);
	}

	t->popIntrptLevel();
}

void Interrupts::defHandler(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	/* do nothing */
	Util::panic("Got interrupt %d (%s) @ %p\n",
			stack->irqNo,intrptList[stack->irqNo & 0x1f].name,stack->r[30]);
}

void Interrupts::exTrap(Thread *t,IntrptStackFrame *stack) {
	t->getStats().syscalls++;
	Syscalls::handle(t,stack);
	/* skip trap-instruction */
	stack->r[30] += 4;
}

void Interrupts::debug(A_UNUSED Thread *t,A_UNUSED IntrptStackFrame *stack) {
	Console::start(NULL);
}

void Interrupts::exPageFault(Thread *t,IntrptStackFrame *stack) {
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

	/* pagefault in kernel? */
	if(t->getIntrptLevel() == 1) {
		/* skip that instruction */
		stack->r[30] += 4;
	}

	pid_t pid = Proc::getRunning();
	Log::get().writef("proc %d, page fault for address %p @ %p\n",pid,pfaddr,stack->r[30]);
	Log::get().writef("Unable to resolve because: %s (%d)\n",strerror(res),res);
#if PANIC_ON_PAGEFAULT
	Util::setpf(pfaddr,stack->r[30]);
	Util::panic("proc %d: page fault for address %p @ %p\n",pid,pfaddr,stack->r[30]);
#else
	Signals::addSignalFor(t,SIG_SEGFAULT);
#endif
}

void Interrupts::irqTimer(Thread *t,A_UNUSED IntrptStackFrame *stack) {
	bool res = fireIrq(stack->irqNo);
	res |= Timer::intrpt();
	Timer::ackIntrpt();
	if(res) {
		if(t->getIntrptLevel() == 0)
			Thread::switchAway();
	}
}

void Interrupts::irqKB(A_UNUSED Thread *t,A_UNUSED IntrptStackFrame *stack) {
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
	if(!fireIrq(stack->irqNo)) {
		/* if there is no device that handles the signal, reenable interrupts */
		kbRegs[KEYBOARD_CTRL] |= KEYBOARD_IEN;
	}
	else
		Thread::switchAway();
}

void Interrupts::irqDisk(A_UNUSED Thread *t,A_UNUSED IntrptStackFrame *stack) {
	/* see Interrupts::irqKb() */
	uint32_t *diskRegs = (uint32_t*)DISK_BASE;
	diskRegs[DISK_CTRL] &= ~DISK_IEN;
	if(!fireIrq(stack->irqNo))
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
