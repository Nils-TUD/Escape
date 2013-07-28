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
#include <sys/task/proc.h>
#include <sys/task/signals.h>
#include <sys/task/timer.h>
#include <sys/task/uenv.h>
#include <sys/mem/paging.h>
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

const Interrupts::Interrupt Interrupts::intrptList[] = {
	/* 0x00: TRAP_POWER_FAILURE */	{defHandler,	"Power failure",		0},
	/* 0x01: TRAP_MEMORY_PARITY */	{defHandler,	"Memory parity error",	0},
	/* 0x02: TRAP_NONEX_MEMORY */	{defHandler,	"Nonexistent memory",	0},
	/* 0x03: TRAP_REBOOT */			{defHandler,	"Reboot",				0},
	/* 0x04: -- */					{defHandler,	"??",					0},
	/* 0x05: -- */					{defHandler,	"??",					0},
	/* 0x06: -- */					{defHandler,	"??",					0},
	/* 0x07: TRAP_INTERVAL */		{defHandler,	"Interval counter",		0},
	/* 0x08: -- */					{defHandler,	"??",					0},
	/* 0x09: -- */					{defHandler,	"??",					0},
	/* 0x0A: -- */					{defHandler,	"??",					0},
	/* 0x0B: -- */					{defHandler,	"??",					0},
	/* 0x0C: -- */					{defHandler,	"??",					0},
	/* 0x0D: -- */					{defHandler,	"??",					0},
	/* 0x0E: -- */					{defHandler,	"??",					0},
	/* 0x0F: -- */					{defHandler,	"??",					0},
	/* 0x10: -- */					{defHandler,	"??",					0},
	/* 0x11: -- */					{defHandler,	"??",					0},
	/* 0x12: -- */					{defHandler,	"??",					0},
	/* 0x13: -- */					{defHandler,	"??",					0},
	/* 0x14: -- */					{defHandler,	"??",					0},
	/* 0x15: -- */					{defHandler,	"??",					0},
	/* 0x16: -- */					{defHandler,	"??",					0},
	/* 0x17: -- */					{defHandler,	"??",					0},
	/* 0x18: -- */					{defHandler,	"??",					0},
	/* 0x19: -- */					{defHandler,	"??",					0},
	/* 0x1A: -- */					{defHandler,	"??",					0},
	/* 0x1B: -- */					{defHandler,	"??",					0},
	/* 0x1C: -- */					{defHandler,	"??",					0},
	/* 0x1D: -- */					{defHandler,	"??",					0},
	/* 0x1E: -- */					{defHandler,	"??",					0},
	/* 0x1F: -- */					{defHandler,	"??",					0},
	/* 0x20: TRAP_PRIVILEGED_PC */	{defHandler,	"Privileged PC",		0},
	/* 0x21: TRAP_SEC_VIOLATION */	{defHandler,	"Security violation",	0},
	/* 0x22: TRAP_BREAKS_RULES */	{defHandler,	"Breaks rules",			0},
	/* 0x23: TRAP_PRIV_INSTR */		{defHandler,	"Privileged instr.",	0},
	/* 0x24: TRAP_PRIV_ACCESS */	{defHandler,	"Privileged access",	0},
	/* 0x25: TRAP_PROT_EXEC */		{exProtFault,	"Execution protection",	0},
	/* 0x26: TRAP_PROT_WRITE */		{exProtFault,	"Write protection",		0},
	/* 0x27: TRAP_PROT_READ */		{exProtFault,	"Read protection",		0},
	/* 0x28: -- */					{defHandler,	"??",					0},
	/* 0x29: -- */					{defHandler,	"??",					0},
	/* 0x2A: -- */					{defHandler,	"??",					0},
	/* 0x2B: -- */					{defHandler,	"??",					0},
	/* 0x2C: -- */					{defHandler,	"??",					0},
	/* 0x2D: -- */					{defHandler,	"??",					0},
	/* 0x2E: -- */					{defHandler,	"??",					0},
	/* 0x2F: -- */					{defHandler,	"??",					0},
	/* 0x30: -- */					{defHandler,	"??",					0},
	/* 0x31: -- */					{defHandler,	"??",					0},
	/* 0x32: -- */					{defHandler,	"??",					0},
	/* 0x33: TRAP_DISK */			{irqDisk,		"Disk",					0},
	/* 0x34: TRAP_TIMER */			{irqTimer,		"Timer",				0},
	/* 0x35: TRAP_TTY0_XMTR */		{defHandler,	"Terminal 0 trans.",	0},
	/* 0x36: TRAP_TTY0_RCVR */		{defHandler,	"Terminal 0 recv.",		0},
	/* 0x37: TRAP_TTY1_XMTR */		{defHandler,	"Terminal 1 trans.",	0},
	/* 0x38: TRAP_TTY1_RCVR */		{defHandler,	"Terminal 1 recv.",		0},
	/* 0x39: TRAP_TTY2_XMTR */		{defHandler,	"Terminal 2 trans.",	0},
	/* 0x3A: TRAP_TTY2_RCVR */		{defHandler,	"Terminal 2 recv.",		0},
	/* 0x3B: TRAP_TTY3_XMTR */		{defHandler,	"Terminal 3 trans.",	0},
	/* 0x3C: TRAP_TTY3_RCVR */		{defHandler,	"Terminal 3 recv.",		0},
	/* 0x3D: TRAP_KEYBOARD */		{irqKB,			"Keyboard",				0},
	/* 0x3E: -- */					{defHandler,	"??",					0},
	/* 0x3F: -- */					{defHandler,	"??",					0},
};
size_t Interrupts::irqCount = 0;

void Interrupts::forcedTrap(IntrptStackFrame *stack) {
	Thread *t = Thread::getRunning();
	enterKernel(t,stack);
	/* calculate offset of registers on the stack */
	uint64_t *begin = stack - (16 + (256 - (stack[-1] >> 56)));
	begin -= *begin;
	t->getStats().syscalls++;
	Syscalls::handle(t,begin);

	/* set $255, which will be put into rSS; the stack-frame changes when cloning */
	t = Thread::getRunning(); /* thread might have changed */
	stack[-14] = DIR_MAPPED_SPACE | (t->getKernelStack() * PAGE_SIZE);

	/* only handle signals, if we come directly from user-mode */
	if((t->getFlags() & T_IDLE) || t->getIntrptLevel() == 0)
		UEnv::handleSignal(t,stack);
	leaveKernel(t);
}

bool Interrupts::dynTrap(IntrptStackFrame *stack,int irqNo) {
	const Interrupt *intrpt;
	Thread *t = Thread::getRunning();
	enterKernel(t,stack);
	irqCount++;

	/* call handler */
	intrpt = intrptList + (irqNo & 0x3F);
	intrpt->handler(stack,irqNo);

	/* only handle signals, if we come directly from user-mode */
	t = Thread::getRunning();
	if((t->getFlags() & T_IDLE) || t->getIntrptLevel() == 0)
		UEnv::handleSignal(t,stack);
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
	if(!VirtMem::pagefault(pfaddr,irqNo == TRAP_PROT_WRITE)) {
		/* ok, now lets check if the thread wants more stack-pages */
		if(Thread::extendStack(pfaddr) < 0) {
			pid_t pid = Proc::getRunning();
			sKSpecRegs *sregs = Thread::getRunning()->getSpecRegs();
			Log::get().writef("proc %d: %s for address %p @ %p\n",pid,intrptList[irqNo].name,
					pfaddr,sregs->rww);
			Proc::segFault();
		}
	}
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
	 * call uenv_handleSignal(), which might cause a thread-switch */
	if(!Signals::addSignal(SIG_INTRPT_KB)) {
		/* if there is no device that handles the signal, reenable interrupts */
		kbRegs[KEYBOARD_CTRL] |= KEYBOARD_IEN;
	}
}

void Interrupts::irqTimer(A_UNUSED IntrptStackFrame *stack,A_UNUSED int irqNo) {
	bool res;
	Signals::addSignal(SIG_INTRPT_TIMER);
	res = Timer::intrpt();
	Timer::ackIntrpt();
	if(res) {
		Thread *t = Thread::getRunning();
		if(t->getIntrptLevel() == 0)
			Thread::switchAway();
	}
}

void Interrupts::irqDisk(A_UNUSED IntrptStackFrame *stack,A_UNUSED int irqNo) {
	/* see interrupt_irqKb() */
	uint64_t *diskRegs = (uint64_t*)DISK_BASE;
	diskRegs[DISK_CTRL] &= ~DISK_IEN;
	if(!Signals::addSignal(SIG_INTRPT_ATA1))
		diskRegs[DISK_CTRL] |= DISK_IEN;
}

void InterruptsBase::printStackFrame(OStream &os,const IntrptStackFrame *stack) {
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
