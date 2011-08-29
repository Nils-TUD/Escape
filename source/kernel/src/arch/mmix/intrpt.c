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
#include <sys/mem/paging.h>
#include <sys/mem/vmm.h>
#include <sys/dbg/kb.h>
#include <sys/dbg/console.h>
#include <esc/keycodes.h>
#include <sys/cpu.h>
#include <sys/syscalls.h>
#include <sys/intrpt.h>
#include <sys/video.h>
#include <sys/util.h>

#define DEBUG_PAGEFAULTS		0

#define TRAP_POWER_FAILURE		0	// power failure
#define TRAP_MEMORY_PARITY		1	// memory parity error
#define TRAP_NONEX_MEMORY		2	// non-existent memory
#define TRAP_REBOOT				3	// reboot
#define TRAP_INTERVAL			7	// interval-counter reached zero

#define TRAP_PRIVILEGED_PC		32	// p: comes from a privileged (negative) virt addr
#define TRAP_SEC_VIOLATION		33	// s: violates security
#define TRAP_BREAKS_RULES		34	// b: breaks the rules of MMIX
#define TRAP_PRIV_INSTR			35	// k: is privileged, for use by the "kernel" only
#define TRAP_PRIV_ACCESS		36	// n: refers to a negative virtual address
#define TRAP_PROT_EXEC			37	// x: appears in a page without execute permission
#define TRAP_PROT_WRITE			38	// w: tries to store to a page without write perm
#define TRAP_PROT_READ			39	// r: tries to load from a page without read perm

#define TRAP_DISK				51	// disk interrupt
#define TRAP_TIMER				52	// timer interrupt
#define TRAP_TTY0_XMTR			53	// terminal 0 transmitter interrupt
#define TRAP_TTY0_RCVR			54	// terminal 0 transmitter interrupt
#define TRAP_TTY1_XMTR			55	// terminal 1 transmitter interrupt
#define TRAP_TTY1_RCVR			56	// terminal 1 transmitter interrupt
#define TRAP_TTY2_XMTR			57	// terminal 2 transmitter interrupt
#define TRAP_TTY2_RCVR			58	// terminal 2 transmitter interrupt
#define TRAP_TTY3_XMTR			59	// terminal 3 transmitter interrupt
#define TRAP_TTY3_RCVR			60	// terminal 3 transmitter interrupt
#define TRAP_KEYBOARD			61	// keyboard interrupt

#define KEYBOARD_BASE			0x8006000000000000
#define KEYBOARD_CTRL			0
#define KEYBOARD_IEN			0x02

#define DISK_BASE				0x8003000000000000
#define DISK_CTRL				0
#define DISK_IEN				0x02

typedef void (*fIntrptHandler)(sIntrptStackFrame *stack,int irqNo);
typedef struct {
	fIntrptHandler handler;
	const char *name;
	sig_t signal;
} sInterrupt;

void intrpt_forcedTrap(sIntrptStackFrame *stack);
void intrpt_dynTrap(sIntrptStackFrame *stack,int irqNo);

static void intrpt_enterKernel(sThread *t,sIntrptStackFrame *stack);
static void intrpt_leaveKernel(sThread *t);
static void intrpt_defHandler(sIntrptStackFrame *stack,int irqNo);
static void intrpt_exProtFault(sIntrptStackFrame *stack,int irqNo);
static void intrpt_irqKB(sIntrptStackFrame *stack,int irqNo);
static void intrpt_irqTimer(sIntrptStackFrame *stack,int irqNo);
static void intrpt_irqDisk(sIntrptStackFrame *stack,int irqNo);

static sInterrupt intrptList[] = {
	/* 0x00: TRAP_POWER_FAILURE */	{intrpt_defHandler,	"Power failure",		0},
	/* 0x01: TRAP_MEMORY_PARITY */	{intrpt_defHandler,	"Memory parity error",	0},
	/* 0x02: TRAP_NONEX_MEMORY */	{intrpt_defHandler,	"Nonexistent memory",	0},
	/* 0x03: TRAP_REBOOT */			{intrpt_defHandler,	"Reboot",				0},
	/* 0x04: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x05: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x06: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x07: TRAP_INTERVAL */		{intrpt_defHandler,	"Interval counter",		0},
	/* 0x08: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x09: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x0A: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x0B: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x0C: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x0D: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x0E: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x0F: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x10: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x11: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x12: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x13: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x14: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x15: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x16: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x17: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x18: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x19: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x1A: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x1B: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x1C: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x1D: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x1E: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x1F: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x20: TRAP_PRIVILEGED_PC */	{intrpt_defHandler,	"Privileged PC",		0},
	/* 0x21: TRAP_SEC_VIOLATION */	{intrpt_defHandler,	"Security violation",	0},
	/* 0x22: TRAP_BREAKS_RULES */	{intrpt_defHandler,	"Breaks rules",			0},
	/* 0x23: TRAP_PRIV_INSTR */		{intrpt_defHandler,	"Privileged instr.",	0},
	/* 0x24: TRAP_PRIV_ACCESS */	{intrpt_defHandler,	"Privileged access",	0},
	/* 0x25: TRAP_PROT_EXEC */		{intrpt_exProtFault,"Execution protection",	0},
	/* 0x26: TRAP_PROT_WRITE */		{intrpt_exProtFault,"Write protection",		0},
	/* 0x27: TRAP_PROT_READ */		{intrpt_exProtFault,"Read protection",		0},
	/* 0x28: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x29: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x2A: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x2B: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x2C: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x2D: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x2E: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x2F: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x30: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x31: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x32: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x33: TRAP_DISK */			{intrpt_irqDisk,	"Disk",					0},
	/* 0x34: TRAP_TIMER */			{intrpt_irqTimer,	"Timer",				0},
	/* 0x35: TRAP_TTY0_XMTR */		{intrpt_defHandler,	"Terminal 0 trans.",	0},
	/* 0x36: TRAP_TTY0_RCVR */		{intrpt_defHandler,	"Terminal 0 recv.",		0},
	/* 0x37: TRAP_TTY1_XMTR */		{intrpt_defHandler,	"Terminal 1 trans.",	0},
	/* 0x38: TRAP_TTY1_RCVR */		{intrpt_defHandler,	"Terminal 1 recv.",		0},
	/* 0x39: TRAP_TTY2_XMTR */		{intrpt_defHandler,	"Terminal 2 trans.",	0},
	/* 0x3A: TRAP_TTY2_RCVR */		{intrpt_defHandler,	"Terminal 2 recv.",		0},
	/* 0x3B: TRAP_TTY3_XMTR */		{intrpt_defHandler,	"Terminal 3 trans.",	0},
	/* 0x3C: TRAP_TTY3_RCVR */		{intrpt_defHandler,	"Terminal 3 recv.",		0},
	/* 0x3D: TRAP_KEYBOARD */		{intrpt_irqKB,		"Keyboard",				0},
	/* 0x3E: -- */					{intrpt_defHandler,	"??",					0},
	/* 0x3F: -- */					{intrpt_defHandler,	"??",					0},
};
static size_t irqCount = 0;

size_t intrpt_getCount(void) {
	return irqCount;
}

void intrpt_forcedTrap(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	intrpt_enterKernel(t,stack);
	/* calculate offset of registers on the stack */
	uint64_t *begin = stack - (16 + (256 - (stack[-1] >> 56)));
	begin -= *begin;
	t->stats.syscalls++;
	sysc_handle(begin);

	/* set $255, which will be put into rSS; the stack-frame changes when cloning */
	t = thread_getRunning(); /* thread might have changed */
	stack[-14] = DIR_MAPPED_SPACE | (t->archAttr.kstackFrame * PAGE_SIZE);

	/* only handle signals, if we come directly from user-mode */
	if((t->flags & T_IDLE) || thread_getIntrptLevel(t) == 0)
		uenv_handleSignal(t,stack);
	intrpt_leaveKernel(t);
}

void intrpt_dynTrap(sIntrptStackFrame *stack,int irqNo) {
	sInterrupt *intrpt;
	sThread *t = thread_getRunning();
	intrpt_enterKernel(t,stack);
	irqCount++;

	/* call handler */
	intrpt = intrptList + (irqNo & 0x3F);
	intrpt->handler(stack,irqNo);

	/* only handle signals, if we come directly from user-mode */
	t = thread_getRunning();
	if((t->flags & T_IDLE) || thread_getIntrptLevel(t) == 0)
		uenv_handleSignal(t,stack);
	intrpt_leaveKernel(t);
}

static void intrpt_enterKernel(sThread *t,sIntrptStackFrame *stack) {
	uint64_t cycles = cpu_rdtsc();
	thread_pushIntrptLevel(t,stack);
	thread_pushSpecRegs();
	if(t->stats.ucycleStart > 0)
		t->stats.ucycleCount.val64 += cycles - t->stats.ucycleStart;
	/* kernel-mode starts here */
	t->stats.kcycleStart = cycles;
}

static void intrpt_leaveKernel(sThread *t) {
	thread_popSpecRegs();
	thread_popIntrptLevel(t);
	/* kernel-mode ends */
	uint64_t cycles = cpu_rdtsc();
	if(t->stats.kcycleStart > 0)
		t->stats.kcycleCount.val64 += cycles - t->stats.kcycleStart;
	/* user-mode starts here */
	t->stats.ucycleStart = cycles;
}

static void intrpt_defHandler(sIntrptStackFrame *stack,int irqNo) {
	UNUSED(stack);
	uint64_t rww = cpu_getSpecial(rWW);
	/* do nothing */
	util_panic("Got interrupt %d (%s) @ %p\n",
			irqNo,intrptList[irqNo & 0x3f].name,rww);
}

static void intrpt_exProtFault(sIntrptStackFrame *stack,int irqNo) {
	UNUSED(stack);
	uintptr_t pfaddr = cpu_getFaultLoc();

#if DEBUG_PAGEFAULTS
	if(pfaddr == lastPFAddr && lastPFProc == proc_getRunning()->pid) {
		exCount++;
		if(exCount >= MAX_EX_COUNT)
			util_panic("%d page-faults at the same address of the same process",exCount);
	}
	else
		exCount = 0;
	lastPFAddr = pfaddr;
	lastPFProc = proc_getRunning()->pid;
	vid_printf("Page fault for address=0x%08x @ 0x%x, process %d\n",pfaddr,
			stack->r[30],proc_getRunning()->pid);
#endif

	/* first let the vmm try to handle the page-fault (demand-loading, cow, swapping, ...) */
	if(!vmm_pagefault(pfaddr)) {
		/* ok, now lets check if the thread wants more stack-pages */
		if(thread_extendStack(pfaddr) < 0) {
			pid_t pid = proc_getRunning();
			sKSpecRegs *sregs = thread_getSpecRegs();
			vid_printf("proc %d: %s for address %p @ %p\n",pid,intrptList[irqNo].name,
					pfaddr,sregs->rww);
			proc_segFault();
		}
	}
}

static void intrpt_irqKB(sIntrptStackFrame *stack,int irqNo) {
	UNUSED(stack);
	UNUSED(irqNo);
	/* we have to disable interrupts until the device has handled the request */
	/* otherwise we would get into an interrupt loop */
	uint64_t *kbRegs = (uint64_t*)KEYBOARD_BASE;
	kbRegs[KEYBOARD_CTRL] &= ~KEYBOARD_IEN;

	if(proc_getByPid(KEYBOARD_PID) == NULL) {
		/* in debug-mode, start the logviewer when the keyboard is not present yet */
		/* (with a present keyboard-driver we would steal him the scancodes) */
		/* this way, we can debug the system in the startup-phase without affecting timings
		 * (before viewing the log ;)) */
		sKeyEvent ev;
		if(kb_get(&ev,KEV_PRESS,false) && ev.keycode == VK_F12)
			cons_start();
	}

	/* we can't add the signal before the kb-interrupts are disabled; otherwise a kernel-miss might
	 * call uenv_handleSignal(), which might cause a thread-switch */
	if(!sig_addSignal(SIG_INTRPT_KB)) {
		/* if there is no driver that handles the signal, reenable interrupts */
		kbRegs[KEYBOARD_CTRL] |= KEYBOARD_IEN;
	}
}

static void intrpt_irqTimer(sIntrptStackFrame *stack,int irqNo) {
	bool res;
	UNUSED(stack);
	UNUSED(irqNo);
	sig_addSignal(SIG_INTRPT_TIMER);
	res = timer_intrpt();
	timer_ackIntrpt();
	if(res) {
		sThread *t = thread_getRunning();
		if(thread_getIntrptLevel(t) == 0)
			thread_switch();
	}
}

static void intrpt_irqDisk(sIntrptStackFrame *stack,int irqNo) {
	UNUSED(stack);
	UNUSED(irqNo);
	/* see interrupt_irqKb() */
	uint64_t *diskRegs = (uint64_t*)DISK_BASE;
	diskRegs[DISK_CTRL] &= ~DISK_IEN;
	if(!sig_addSignal(SIG_INTRPT_ATA1))
		diskRegs[DISK_CTRL] |= DISK_IEN;
}

void intrpt_printStackFrame(const sIntrptStackFrame *stack) {
	stack--;
	int i,j,rl,rg = *stack >> 56;
	int changedStack = (*stack >> 32) & 0x1;
	static int spregs[] = {rZ,rY,rX,rW,rP,rR,rM,rJ,rH,rE,rD,rB};
	vid_printf("\trG=%d,rA=#%x\n",rg,*stack-- & 0x3FFFF);
	vid_printf("\t");
	for(j = 0, i = 1; i <= (int)ARRAY_SIZE(spregs); j++, i++) {
		vid_printf("%-4s: #%016lx ",cpu_getSpecialName(spregs[i - 1]),*stack--);
		if(j % 3 == 2)
			vid_printf("\n\t");
	}
	if(j % 3 != 0)
		vid_printf("\n\t");
	for(j = 0, i = 255; i >= rg; j++, i--) {
		vid_printf("$%-3d: #%016lx ",i,*stack--);
		if(j % 3 == 2)
			vid_printf("\n\t");
	}
	if(j % 3 != 0)
		vid_printf("\n\t");
	if(changedStack) {
		vid_printf("rS  : #%016lx",*stack--);
		vid_printf(" rO  : #%016lx\n\t",*stack--);
	}
	rl = *stack--;
	vid_printf("rL  : %d\n\t",rl);
	for(j = 0, i = rl - 1; i >= 0; j++, i--) {
		vid_printf("$%-3d: #%016lx ",i,*stack--);
		if(i > 0 && j % 3 == 2)
			vid_printf("\n\t");
	}
	vid_printf("\n");
}
