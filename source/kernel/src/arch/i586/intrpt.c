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
#include <sys/arch/i586/apic.h>
#include <sys/dbg/kb.h>
#include <sys/dbg/console.h>
#include <sys/mem/cache.h>
#include <sys/mem/swap.h>
#include <sys/mem/vmm.h>
#include <sys/task/signals.h>
#include <sys/task/smp.h>
#include <sys/task/thread.h>
#include <sys/task/uenv.h>
#include <sys/task/timer.h>
#include <sys/syscalls.h>
#include <sys/cpu.h>
#include <sys/intrpt.h>
#include <sys/util.h>
#include <sys/video.h>
#include <esc/keycodes.h>
#include <esc/syscalls.h>

#define DEBUG_PAGEFAULTS		0

/* maximum number of a exception in a row */
#define MAX_EX_COUNT			10

typedef void (*fIntrptHandler)(sThread *t,sIntrptStackFrame *stack);
typedef struct {
	fIntrptHandler handler;
	const char *name;
	sig_t signal;
} sInterrupt;

/**
 * The exception and interrupt-handlers
 */
static void intrpt_exFatal(sThread *t,sIntrptStackFrame *stack);
static void intrpt_exGenProtFault(sThread *t,sIntrptStackFrame *stack);
static void intrpt_exCoProcNA(sThread *t,sIntrptStackFrame *stack);
static void intrpt_exPageFault(sThread *t,sIntrptStackFrame *stack);
static void intrpt_irqTimer(sThread *t,sIntrptStackFrame *stack);
static void intrpt_irqDefault(sThread *t,sIntrptStackFrame *stack);
static void intrpt_syscall(sThread *t,sIntrptStackFrame *stack);
static void intrpt_ipiWork(sThread *t,sIntrptStackFrame *stack);
static void intrpt_ipiTerm(sThread *t,sIntrptStackFrame *stack);
static void intrpt_printPFInfo(sIntrptStackFrame *stack,uintptr_t pfaddr);

static const sInterrupt intrptList[] = {
	/* 0x00: EX_DIVIDE_BY_ZERO */		{intrpt_exFatal,"Divide by zero",0},
	/* 0x01: EX_SINGLE_STEP */			{intrpt_exFatal,"Single step",0},
	/* 0x02: EX_NONMASKABLE */			{intrpt_exFatal,"Non maskable",0},
	/* 0x03: EX_BREAKPOINT */			{intrpt_exFatal,"Breakpoint",0},
	/* 0x04: EX_OVERFLOW */				{intrpt_exFatal,"Overflow",0},
	/* 0x05: EX_BOUNDS_CHECK */			{intrpt_exFatal,"Bounds check",0},
	/* 0x06: EX_INVALID_OPCODE */		{intrpt_exFatal,"Invalid opcode",0},
	/* 0x07: EX_CO_PROC_NA */			{intrpt_exCoProcNA,"Co-processor not available",0},
	/* 0x08: EX_DOUBLE_FAULT */			{intrpt_exFatal,"Double fault",0},
	/* 0x09: EX_CO_PROC_SEG_OVR */		{intrpt_exFatal,"Co-processor segment overrun",0},
	/* 0x0A: EX_INVALID_TSS */			{intrpt_exFatal,"Invalid TSS",0},
	/* 0x0B: EX_SEG_NOT_PRESENT */		{intrpt_exFatal,"Segment not present",0},
	/* 0x0C: EX_STACK */				{intrpt_exFatal,"Stack exception",0},
	/* 0x0D: EX_GEN_PROT_FAULT */		{intrpt_exGenProtFault,"General protection fault",0},
	/* 0x0E: EX_PAGE_FAULT */			{intrpt_exPageFault,"Page fault",0},
	/* 0x0F: -- */						{NULL,"Unknown",0},
	/* 0x10: EX_CO_PROC_ERROR */		{intrpt_exFatal,"Co-processor error",0},
	/* 0x11: -- */						{NULL,"Unknown",0},
	/* 0x12: -- */						{NULL,"Unknown",0},
	/* 0x13: -- */						{NULL,"Unknown",0},
	/* 0x14: -- */						{NULL,"Unknown",0},
	/* 0x15: -- */						{NULL,"Unknown",0},
	/* 0x16: -- */						{NULL,"Unknown",0},
	/* 0x17: -- */						{NULL,"Unknown",0},
	/* 0x18: -- */						{NULL,"Unknown",0},
	/* 0x19: -- */						{NULL,"Unknown",0},
	/* 0x1A: -- */						{NULL,"Unknown",0},
	/* 0x1B: -- */						{NULL,"Unknown",0},
	/* 0x1C: -- */						{NULL,"Unknown",0},
	/* 0x1D: -- */						{NULL,"Unknown",0},
	/* 0x1E: -- */						{NULL,"Unknown",0},
	/* 0x1F: -- */						{NULL,"Unknown",0},
	/* 0x20: IRQ_TIMER */				{intrpt_irqTimer,"Timer",SIG_INTRPT_TIMER},
	/* 0x21: IRQ_KEYBOARD */			{intrpt_irqDefault,"Keyboard",SIG_INTRPT_KB},
	/* 0x22: -- */						{NULL,"Unknown",0},
	/* 0x23: IRQ_COM2 */				{intrpt_irqDefault,"COM2",SIG_INTRPT_COM2},
	/* 0x24: IRQ_COM1 */				{intrpt_irqDefault,"COM1",SIG_INTRPT_COM1},
	/* 0x25: -- */						{NULL,"Unknown",0},
	/* 0x26: IRQ_FLOPPY */				{intrpt_irqDefault,"Floppy",SIG_INTRPT_FLOPPY},
	/* 0x27: -- */						{NULL,"Unknown",0},
	/* 0x28: IRQ_CMOS_RTC */			{intrpt_irqDefault,"CMOS",SIG_INTRPT_CMOS},
	/* 0x29: -- */						{NULL,"Unknown",0},
	/* 0x2A: -- */						{NULL,"Unknown",0},
	/* 0x2B: -- */						{NULL,"Unknown",0},
	/* 0x2C: IRQ_MOUSE */				{intrpt_irqDefault,"Mouse",SIG_INTRPT_MOUSE},
	/* 0x2D: -- */						{NULL,"Unknown",0},
	/* 0x2C: IRQ_ATA1 */				{intrpt_irqDefault,"ATA1",SIG_INTRPT_ATA1},
	/* 0x2C: IRQ_ATA2 */				{intrpt_irqDefault,"ATA2",SIG_INTRPT_ATA2},
	/* 0x30: syscall */					{intrpt_syscall,"Systemcall",0},
	/* 0x31: IPI_WORK */				{intrpt_ipiWork,"Work IPI",0},
	/* 0x32: IPI_TERM */				{intrpt_ipiTerm,"Term IPI",0},
	/* 0x33: IPI_FLUSH_TLB */			{intrpt_exFatal,"Flush TLB IPI",0},	/* not handled here */
	/* 0x34: IPI_WAIT */				{intrpt_exFatal,"",0},
	/* 0x35: IPI_HALT */				{intrpt_exFatal,"",0},
};

/* total number of interrupts */
static size_t intrptCount = 0;

static uintptr_t *pfAddrs;
/* stuff to count exceptions */
static size_t exCount = 0;
static uint32_t lastEx = 0xFFFFFFFF;
#if DEBUG_PAGEFAULTS
static uintptr_t lastPFAddr = ~(uintptr_t)0;
static pid_t lastPFProc = INVALID_PID;
#endif

void intrpt_init(void) {
	pfAddrs = cache_alloc(smp_getCPUCount() * sizeof(uintptr_t));
	if(pfAddrs == NULL)
		util_panic("Unable to alloc memory for pagefault-addresses");
}

size_t intrpt_getCount(void) {
	return intrptCount;
}

uint8_t intrpt_getVectorFor(uint8_t irq) {
	return irq + 0x20;
}

void intrpt_handler(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	const sInterrupt *intrpt;
	size_t level = thread_pushIntrptLevel(t,stack);
	intrptCount++;

	/* we need to save the page-fault address here because swapping may cause other ones */
	if(stack->intrptNo == EX_PAGE_FAULT)
		pfAddrs[t->cpu] = cpu_getCR2();

	if(level == 1)
		swap_check();

	intrpt = intrptList + stack->intrptNo;
	if(intrpt->handler)
		intrpt->handler(t,stack);
	else {
		vid_printf("Got interrupt %d (%s) @ 0x%x in process %d (%s)\n",stack->intrptNo,
				intrpt->name,stack->eip,t->proc->pid,t->proc->command);
	}

	/* handle signal */
	t = thread_getRunning();
	if(level == 1)
		uenv_handleSignal(t,stack);

	thread_popIntrptLevel(t);
}

static void intrpt_exFatal(__attribute__((unused)) sThread *t,sIntrptStackFrame *stack) {
	/* count consecutive occurrences */
	if(lastEx == stack->intrptNo) {
		exCount++;

		/* stop here? */
		if(exCount >= MAX_EX_COUNT) {
			/* for exceptions in kernel: ensure that we have the default print-function */
			if(stack->eip >= KERNEL_AREA)
				vid_unsetPrintFunc();
			util_panic("Got this exception (0x%x) %d times. Stopping here (@ 0x%x)\n",
					stack->intrptNo,exCount,stack->eip);
		}
	}
	else {
		exCount = 0;
		lastEx = stack->intrptNo;
	}
}

static void intrpt_exGenProtFault(sThread *t,sIntrptStackFrame *stack) {
	/* for exceptions in kernel: ensure that we have the default print-function */
	if(stack->eip >= KERNEL_AREA)
		vid_unsetPrintFunc();
	/* io-map not loaded yet? */
	if(ioports_handleGPF(t->proc->pid)) {
		exCount = 0;
		return;
	}
	/* vm86-task? */
	if(t->proc->flags & P_VM86) {
		vm86_handleGPF(stack);
		exCount = 0;
		return;
	}
	/* TODO later the process should be killed here */
	util_panic("GPF @ 0x%x",stack->eip);
}

static void intrpt_exCoProcNA(sThread *t,A_UNUSED sIntrptStackFrame *stack) {
	fpu_handleCoProcNA(&t->archAttr.fpuState);
}

static void intrpt_exPageFault(sThread *t,sIntrptStackFrame *stack) {
	uintptr_t addr = pfAddrs[t->cpu];
	/* for exceptions in kernel: ensure that we have the default print-function */
	if(stack->eip >= KERNEL_AREA)
		vid_unsetPrintFunc();

#if DEBUG_PAGEFAULTS
	if(addr == lastPFAddr && lastPFProc == proc_getRunning()) {
		exCount++;
		if(exCount >= MAX_EX_COUNT)
			util_panic("%d page-faults at the same address of the same process",exCount);
	}
	else
		exCount = 0;
	lastPFAddr = addr;
	lastPFProc = proc_getRunning();
	intrpt_printPFInfo(stack,addr);
#endif

	/* first let the vmm try to handle the page-fault (demand-loading, cow, swapping, ...) */
	if(!vmm_pagefault(addr)) {
		/* ok, now lets check if the thread wants more stack-pages */
		if(thread_extendStack(addr) < 0) {
			intrpt_printPFInfo(stack,addr);
			proc_segFault();
		}
	}
}

static void intrpt_irqTimer(sThread *t,sIntrptStackFrame *stack) {
	bool res;
	const sInterrupt *intrpt = intrptList + stack->intrptNo;
	if(intrpt->signal)
		sig_addSignal(intrpt->signal);
	res = timer_intrpt();
	pic_eoi(stack->intrptNo);
	if(res) {
		if(thread_getIntrptLevel(t) == 0)
			thread_switch();
	}
}

static void intrpt_irqDefault(A_UNUSED sThread *t,sIntrptStackFrame *stack) {
	const sInterrupt *intrpt = intrptList + stack->intrptNo;
	if(intrpt->signal)
		sig_addSignal(intrpt->signal);
	pic_eoi(stack->intrptNo);
	/* in debug-mode, start the logviewer when the keyboard is not present yet */
	/* (with a present keyboard-driver we would steal him the scancodes) */
	/* this way, we can debug the system in the startup-phase without affecting timings
	 * (before viewing the log ;)) */
	if(stack->intrptNo == IRQ_KEYBOARD && proc_getByPid(KEYBOARD_PID) == NULL) {
		sKeyEvent ev;
		if(kb_get(&ev,KEV_PRESS,false) && ev.keycode == VK_F12)
			cons_start();
	}
}

static void intrpt_syscall(sThread *t,sIntrptStackFrame *stack) {
	t->stats.syscalls++;
	sysc_handle(t,stack);
}

static void intrpt_ipiWork(sThread *t,A_UNUSED sIntrptStackFrame *stack) {
	apic_eoi();
	/* if we have been waked up, but are not idling anymore, wakeup another cpu */
	/* this may happen if we're about to switch to a non-idle-thread and have idled previously,
	 * while smp_wakeupCPU() was called. */
	if(!(t->flags & T_IDLE))
		smp_wakeupCPU();
	/* otherwise switch to non-idle-thread (if there is any) */
	else
		thread_switch();
}

static void intrpt_ipiTerm(sThread *t,A_UNUSED sIntrptStackFrame *stack) {
	/* stop running the thread, if it should die */
	if(t->newState == ST_ZOMBIE)
		thread_switch();
}

static void intrpt_printPFInfo(sIntrptStackFrame *stack,uintptr_t addr) {
	pid_t pid = proc_getRunning();
	vid_printf("Page fault for address %p @ %p, process %d\n",addr,stack->eip,pid);
	vid_printf("Occurred because:\n\t%s\n\t%s\n\t%s\n\t%s%s\n",
			(stack->errorCode & 0x1) ?
				"page-level protection violation" : "not-present page",
			(stack->errorCode & 0x2) ? "write" : "read",
			(stack->errorCode & 0x4) ? "user-mode" : "kernel-mode",
			(stack->errorCode & 0x8) ? "reserved bits set to 1\n\t" : "",
			(stack->errorCode & 0x16) ? "instruction-fetch" : "");
}

#if DEBUGGING

void intrpt_printStackFrame(const sIntrptStackFrame *stack) {
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

#endif
