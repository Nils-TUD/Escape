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
#include <sys/task/proc.h>
#include <sys/task/signals.h>
#include <sys/task/timer.h>
#include <sys/task/uenv.h>
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

#define DEBUG_PAGEFAULTS		0

InterruptsBase::Interrupt InterruptsBase::intrptList[] = {
	/* 0x00 */	{Interrupts::defHandler,	"Power failure",		0},
	/* 0x01 */	{Interrupts::defHandler,	"Memory parity error",	0},
	/* 0x02 */	{Interrupts::defHandler,	"Nonexistent memory",	0},
	/* 0x03 */	{Interrupts::defHandler,	"Reboot",				0},
	/* 0x04 */	{Interrupts::defHandler,	"??",					0},
	/* 0x05 */	{Interrupts::defHandler,	"??",					0},
	/* 0x06 */	{Interrupts::defHandler,	"??",					0},
	/* 0x07 */	{Interrupts::defHandler,	"Interval counter",		0},
	/* 0x08 */	{Interrupts::defHandler,	"??",					0},
	/* 0x09 */	{Interrupts::defHandler,	"??",					0},
	/* 0x0A */	{Interrupts::defHandler,	"??",					0},
	/* 0x0B */	{Interrupts::defHandler,	"??",					0},
	/* 0x0C */	{Interrupts::defHandler,	"??",					0},
	/* 0x0D */	{Interrupts::defHandler,	"??",					0},
	/* 0x0E */	{Interrupts::defHandler,	"??",					0},
	/* 0x0F */	{Interrupts::defHandler,	"??",					0},
	/* 0x10 */	{Interrupts::defHandler,	"??",					0},
	/* 0x11 */	{Interrupts::defHandler,	"??",					0},
	/* 0x12 */	{Interrupts::defHandler,	"??",					0},
	/* 0x13 */	{Interrupts::defHandler,	"??",					0},
	/* 0x14 */	{Interrupts::defHandler,	"??",					0},
	/* 0x15 */	{Interrupts::defHandler,	"??",					0},
	/* 0x16 */	{Interrupts::defHandler,	"??",					0},
	/* 0x17 */	{Interrupts::defHandler,	"??",					0},
	/* 0x18 */	{Interrupts::defHandler,	"??",					0},
	/* 0x19 */	{Interrupts::defHandler,	"??",					0},
	/* 0x1A */	{Interrupts::defHandler,	"??",					0},
	/* 0x1B */	{Interrupts::defHandler,	"??",					0},
	/* 0x1C */	{Interrupts::defHandler,	"??",					0},
	/* 0x1D */	{Interrupts::defHandler,	"??",					0},
	/* 0x1E */	{Interrupts::defHandler,	"??",					0},
	/* 0x1F */	{Interrupts::defHandler,	"??",					0},
	/* 0x20 */	{Interrupts::defHandler,	"Privileged PC",		0},
	/* 0x21 */	{Interrupts::defHandler,	"Security violation",	0},
	/* 0x22 */	{Interrupts::defHandler,	"Breaks rules",			0},
	/* 0x23 */	{Interrupts::defHandler,	"Privileged instr.",	0},
	/* 0x24 */	{Interrupts::defHandler,	"Privileged access",	0},
	/* 0x25 */	{Interrupts::exProtFault,	"Execution prot.",		0},
	/* 0x26 */	{Interrupts::exProtFault,	"Write prot.",			0},
	/* 0x27 */	{Interrupts::exProtFault,	"Read prot.",			0},
	/* 0x28 */	{Interrupts::defHandler,	"??",					0},
	/* 0x29 */	{Interrupts::defHandler,	"??",					0},
	/* 0x2A */	{Interrupts::defHandler,	"??",					0},
	/* 0x2B */	{Interrupts::defHandler,	"??",					0},
	/* 0x2C */	{Interrupts::defHandler,	"??",					0},
	/* 0x2D */	{Interrupts::defHandler,	"??",					0},
	/* 0x2E */	{Interrupts::defHandler,	"??",					0},
	/* 0x2F */	{Interrupts::defHandler,	"??",					0},
	/* 0x30 */	{Interrupts::defHandler,	"??",					0},
	/* 0x31 */	{Interrupts::defHandler,	"??",					0},
	/* 0x32 */	{Interrupts::defHandler,	"??",					0},
	/* 0x33 */	{Interrupts::irqDisk,		"Disk",					0},
	/* 0x34 */	{Interrupts::irqTimer,		"Timer",				0},
	/* 0x35 */	{Interrupts::defHandler,	"Terminal 0 trans.",	0},
	/* 0x36 */	{Interrupts::defHandler,	"Terminal 0 recv.",		0},
	/* 0x37 */	{Interrupts::defHandler,	"Terminal 1 trans.",	0},
	/* 0x38 */	{Interrupts::defHandler,	"Terminal 1 recv.",		0},
	/* 0x39 */	{Interrupts::defHandler,	"Terminal 2 trans.",	0},
	/* 0x3A */	{Interrupts::defHandler,	"Terminal 2 recv.",		0},
	/* 0x3B */	{Interrupts::defHandler,	"Terminal 3 trans.",	0},
	/* 0x3C */	{Interrupts::defHandler,	"Terminal 3 recv.",		0},
	/* 0x3D */	{Interrupts::irqKB,			"Keyboard",				0},
	/* 0x3E */	{Interrupts::defHandler,	"??",					0},
	/* 0x3F */	{Interrupts::defHandler,	"??",					0},
};

int InterruptsBase::installHandler(int irq,const char *) {
	if(irq < 0x32 || irq >= 0x40)
		return -EINVAL;

	if(intrptList[irq].handler == Interrupts::defHandler)
		return -EINVAL;

	/* nothing to do. already installed */
	return 0;
}

void InterruptsBase::uninstallHandler(int irq) {
	/* nothing to do */
}

void Interrupts::forcedTrap(IntrptStackFrame *stack) {
	static_assert(IRQ_COUNT == ARRAY_SIZE(intrptList),"IRQ_COUNT is wrong");
	Thread *t = Thread::getRunning();
	enterKernel(t,stack);
	/* calculate offset of registers on the stack */
	uint64_t *begin = stack - (16 + (256 - (stack[-1] >> 56)));
	begin -= *begin;
	t->getStats().syscalls++;
	Syscalls::handle(t,begin);

	/* set $255, which will be put into rSS; the stack-frame changes when cloning */
	t = Thread::getRunning(); /* thread might have changed */
	stack[-14] = DIR_MAP_AREA | (t->getKernelStack() * PAGE_SIZE);

	/* only handle signals, if we come directly from user-mode */
	if(t->hasSignal() && ((t->getFlags() & T_IDLE) || t->getIntrptLevel() == 0))
		UEnv::handleSignal(t,stack);
	leaveKernel(t);
}

bool Interrupts::dynTrap(IntrptStackFrame *stack,int irqNo) {
	Interrupt *intrpt;
	Thread *t = Thread::getRunning();
	enterKernel(t,stack);

	/* call handler */
	intrpt = intrptList + (irqNo & 0x3F);
	intrpt->count++;
	intrpt->handler(stack,irqNo);

	/* only handle signals, if we come directly from user-mode */
	t = Thread::getRunning();
	if((t->getFlags() & T_IDLE) || t->getIntrptLevel() == 0) {
		if(t->haveHigherPrio())
			Thread::switchAway();
		if(t->hasSignal())
			UEnv::handleSignal(t,stack);
	}
	leaveKernel(t);
	return t->getFlags() & T_IDLE;
}

void Interrupts::enterKernel(Thread *t,IntrptStackFrame *stack) {
	t->pushIntrptLevel(stack);
	Thread::pushSpecRegs();
}

void Interrupts::leaveKernel(Thread *t) {
	Thread::popSpecRegs();
	t->popIntrptLevel();
}

void Interrupts::defHandler(A_UNUSED IntrptStackFrame *stack,int irqNo) {
	uint64_t rww = CPU::getSpecial(rWW);
	/* do nothing */
	Util::panic("Got interrupt %d (%s) @ %p",
			irqNo,intrptList[irqNo & 0x3f].name,rww);
}

void Interrupts::exProtFault(A_UNUSED IntrptStackFrame *stack,int irqNo) {
	uintptr_t pfaddr = CPU::getFaultLoc();

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
	Log::get().writef("Page fault for address=0x%08x @ 0x%x, process %d\n",pfaddr,
			stack->r[30],Proc::getRunning()->getPid());
#endif

	/* first let the vmm try to handle the page-fault (demand-loading, cow, swapping, ...) */
	int res = VirtMem::pagefault(pfaddr,irqNo == TRAP_PROT_WRITE);
	if(EXPECT_TRUE(res == 0))
		return;
	/* ok, now lets check if the thread wants more stack-pages */
	if(EXPECT_TRUE(res == -EFAULT && (res = Thread::extendStack(pfaddr)) == 0))
		return;

	/* pagefault in kernel? */
	Thread *t = Thread::getRunning();
	if(t->getIntrptLevel() == 1) {
		/* skip that instruction */
		KSpecRegs *sregs = t->getSpecRegs();
		sregs->rxx = 1UL << 63;
	}

	pid_t pid = Proc::getRunning();
	KSpecRegs *sregs = t->getSpecRegs();
	Log::get().writef("proc %d: %s for address %p @ %p\n",pid,intrptList[irqNo].name,pfaddr,sregs->rww);
	Log::get().writef("Unable to resolve because: %s (%d)\n",strerror(res),res);
#if PANIC_ON_PAGEFAULT
	Util::setpf(pfaddr,sregs->rww);
	Util::panic("proc %d: %s for address %p @ %p\n",pid,intrptList[irqNo].name,pfaddr,sregs->rww);
#else
	Signals::addSignalFor(Thread::getRunning(),SIG_SEGFAULT);
#endif
}

void Interrupts::irqKB(A_UNUSED IntrptStackFrame *stack,A_UNUSED int irqNo) {
	/* we have to disable interrupts until the device has handled the request */
	/* otherwise we would get into an interrupt loop */
	uint64_t *kbRegs = (uint64_t*)KEYBOARD_BASE;
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
	if(!fireIrq(irqNo)) {
		/* if there is no device that handles the signal, reenable interrupts */
		kbRegs[KEYBOARD_CTRL] |= KEYBOARD_IEN;
	}
	else
		Thread::switchAway();
}

void Interrupts::irqTimer(A_UNUSED IntrptStackFrame *stack,A_UNUSED int irqNo) {
	bool res = fireIrq(irqNo);
	res |= Timer::intrpt();
	Timer::ackIntrpt();
	if(res)
		Thread::switchAway();
}

void Interrupts::irqDisk(A_UNUSED IntrptStackFrame *stack,A_UNUSED int irqNo) {
	/* see Interrupts::irqKb() */
	uint64_t *diskRegs = (uint64_t*)DISK_BASE;
	diskRegs[DISK_CTRL] &= ~DISK_IEN;
	if(!fireIrq(irqNo))
		diskRegs[DISK_CTRL] |= DISK_IEN;
	else
		Thread::switchAway();
}

void Interrupts::printStackFrame(OStream &os,const IntrptStackFrame *stack) {
	stack--;
	int i,j,rl,rg = *stack >> 56;
	int changedStack = (*stack >> 32) & 0x1;
	static int spregs[] = {rZ,rY,rX,rW,rP,rR,rM,rJ,rH,rE,rD,rB};
	os.writef("rG=%d,rA=#%x\n",rg,*stack-- & 0x3FFFF);
	for(j = 0, i = 1; i <= (int)ARRAY_SIZE(spregs); j++, i++) {
		os.writef("%-4s: #%016lx ",CPU::getSpecialName(spregs[i - 1]),*stack--);
		if(j % 3 == 2)
			os.writef("\n");
	}
	if(j % 3 != 0)
		os.writef("\n");
	for(j = 0, i = 255; i >= rg; j++, i--) {
		os.writef("$%-3d: #%016lx ",i,*stack--);
		if(j % 3 == 2)
			os.writef("\n");
	}
	if(j % 3 != 0)
		os.writef("\n");
	if(changedStack) {
		os.writef("rS  : #%016lx",*stack--);
		os.writef(" rO  : #%016lx\n",*stack--);
	}
	rl = *stack--;
	os.writef("rL  : %d\n",rl);
	for(j = 0, i = rl - 1; i >= 0; j++, i--) {
		os.writef("$%-3d: #%016lx ",i,*stack--);
		if(i > 0 && j % 3 == 2)
			os.writef("\n");
	}
	os.writef("\n");
}
