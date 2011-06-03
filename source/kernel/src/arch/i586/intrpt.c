/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <sys/arch/i586/fpu.h>
#include <sys/arch/i586/gdt.h>
#include <sys/arch/i586/task/vm86.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/mem/swap.h>
#include <sys/mem/vmm.h>
#include <sys/task/signals.h>
#include <sys/task/proc.h>
#include <sys/task/elf.h>
#include <sys/task/sched.h>
#include <sys/task/timer.h>
#include <sys/task/uenv.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/real.h>
#include <sys/dbg/kb.h>
#include <sys/dbg/console.h>
#include <sys/cpu.h>
#include <sys/intrpt.h>
#include <sys/util.h>
#include <sys/syscalls.h>
#include <sys/video.h>
#include <esc/sllist.h>
#include <esc/keycodes.h>
#include <esc/syscalls.h>
#include <assert.h>
#include <string.h>
#include <errors.h>

#define DEBUG_PAGEFAULTS		0

#define IDT_COUNT				256
/* the privilege level */
#define IDT_DPL_KERNEL			0
#define IDT_DPL_USER			3
/* reserved by intel */
#define IDT_INTEL_RES1			2
#define IDT_INTEL_RES2			15
/* the code-selector */
#define IDT_CODE_SEL			0x8

/* I/O ports for PICs */
#define PIC_MASTER				0x20				/* base-port for master PIC */
#define PIC_SLAVE				0xA0				/* base-port for slave PIC */
#define PIC_MASTER_CMD			PIC_MASTER			/* command-port for master PIC */
#define PIC_MASTER_DATA			(PIC_MASTER + 1)	/* data-port for master PIC */
#define PIC_SLAVE_CMD			PIC_SLAVE			/* command-port for slave PIC */
#define PIC_SLAVE_DATA			(PIC_SLAVE + 1)		/* data-port for slave PIC */

#define PIC_EOI					0x20				/* end of interrupt */

/* flags in Initialization Command Word 1 (ICW1) */
#define ICW1_NEED_ICW4			0x01				/* ICW4 needed */
#define ICW1_SINGLE				0x02				/* Single (not cascade) mode */
#define ICW1_INTERVAL4			0x04				/* Call address interval 4 (instead of 8) */
#define ICW1_LEVEL				0x08				/* Level triggered (not edge) mode */
#define ICW1_INIT				0x10				/* Initialization - required! */

/* flags in Initialization Command Word 4 (ICW4) */
#define ICW4_8086				0x01				/* 8086/88 (instead of MCS-80/85) mode */
#define ICW4_AUTO				0x02				/* Auto (instead of normal) EOI */
#define ICW4_BUF_SLAVE			0x08				/* Buffered mode/slave */
#define ICW4_BUF_MASTER			0x0C				/* Buffered mode/master */
#define ICW4_SFNM				0x10				/* Special fully nested */

/* maximum number of a exception in a row */
#define MAX_EX_COUNT			10

/* the maximum length of messages (for interrupt-listeners) */
#define MSG_MAX_LEN				8

/* represents an IDT-entry */
typedef struct {
	/* The address[0..15] of the ISR */
	uint16_t offsetLow;
	/* Code selector that the ISR will use */
	uint16_t selector;
	/* these bits are fix: 0.1110.0000.0000b */
	uint16_t fix		: 13,
	/* the privilege level, 00 = ring0, 01 = ring1, 10 = ring2, 11 = ring3 */
	dpl			: 2,
	/* If Present is not set to 1, an exception will occur */
	present		: 1;
	/* The address[16..31] of the ISR */
	uint16_t	offsetHigh;
} A_PACKED sIDTEntry;

/* represents an IDT-pointer */
typedef struct {
	uint16_t size;
	uint32_t address;
} A_PACKED sIDTPtr;

typedef void (*fIntrptHandler)(sIntrptStackFrame *stack);
typedef struct {
	fIntrptHandler handler;
	const char *name;
	tSig signal;
} sInterrupt;

/* isr prototype */
typedef void (*fISR)(void);

/**
 * Assembler routine to load an IDT
 */
extern void intrpt_loadidt(sIDTPtr *idt);

/**
 * Our ISRs
 */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);
extern void isr32(void);
extern void isr33(void);
extern void isr34(void);
extern void isr35(void);
extern void isr36(void);
extern void isr37(void);
extern void isr38(void);
extern void isr39(void);
extern void isr40(void);
extern void isr41(void);
extern void isr42(void);
extern void isr43(void);
extern void isr44(void);
extern void isr45(void);
extern void isr46(void);
extern void isr47(void);
extern void isr48(void);
/* the handler for a other interrupts */
extern void isrNull(void);

/**
 * Inits the programmable interrupt controller
 */
static void intrpt_initPic(void);

/**
 * Sets the IDT-entry for the given interrupt
 *
 * @param number the interrupt-number
 * @param handler the ISR
 * @param dpl the privilege-level
 */
static void intrpt_setIDT(size_t number,fISR handler,uint8_t dpl);

/**
 * Sends EOI to the PIC, if necessary
 *
 * @param intrptNo the interrupt-number
 */
static void intrpt_eoi(uint32_t intrptNo);

/**
 * The exception and interrupt-handlers
 */
static void intrpt_exFatal(sIntrptStackFrame *stack);
static void intrpt_exGenProtFault(sIntrptStackFrame *stack);
static void intrpt_exCoProcNA(sIntrptStackFrame *stack);
static void intrpt_exPageFault(sIntrptStackFrame *stack);
static void intrpt_irqTimer(sIntrptStackFrame *stack);
static void intrpt_irqIgnore(sIntrptStackFrame *stack);
static void intrpt_syscall(sIntrptStackFrame *stack);

static sInterrupt intrptList[] = {
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
	/* 0x21: IRQ_KEYBOARD */			{intrpt_irqIgnore,"Keyboard",SIG_INTRPT_KB},
	/* 0x22: -- */						{NULL,"Unknown",0},
	/* 0x23: IRQ_COM2 */				{intrpt_irqIgnore,"COM2",SIG_INTRPT_COM2},
	/* 0x24: IRQ_COM1 */				{intrpt_irqIgnore,"COM1",SIG_INTRPT_COM1},
	/* 0x25: -- */						{NULL,"Unknown",0},
	/* 0x26: IRQ_FLOPPY */				{intrpt_irqIgnore,"Floppy",SIG_INTRPT_FLOPPY},
	/* 0x27: -- */						{NULL,"Unknown",0},
	/* 0x28: IRQ_CMOS_RTC */			{intrpt_irqIgnore,"CMOS",SIG_INTRPT_CMOS},
	/* 0x29: -- */						{NULL,"Unknown",0},
	/* 0x2A: -- */						{NULL,"Unknown",0},
	/* 0x2B: -- */						{NULL,"Unknown",0},
	/* 0x2C: IRQ_MOUSE */				{intrpt_irqIgnore,"Mouse",SIG_INTRPT_MOUSE},
	/* 0x2D: -- */						{NULL,"Unknown",0},
	/* 0x2C: IRQ_ATA1 */				{intrpt_irqIgnore,"ATA1",SIG_INTRPT_ATA1},
	/* 0x2C: IRQ_ATA2 */				{intrpt_irqIgnore,"ATA2",SIG_INTRPT_ATA2},
	/* 0x30: syscall */					{intrpt_syscall,"Systemcall",0},
};

/* total number of interrupts */
static size_t intrptCount = 0;

static uintptr_t pfaddr = 0;
/* stuff to count exceptions */
static size_t exCount = 0;
static uint32_t lastEx = 0xFFFFFFFF;
#if DEBUG_PAGEFAULTS
static uintptr_t lastPFAddr = ~(uintptr_t)0;
static tPid lastPFProc = INVALID_PID;
#endif

/* pointer to the current interrupt-stack */
static sIntrptStackFrame *curIntrptStack = NULL;

/* the interrupt descriptor table */
static sIDTEntry idt[IDT_COUNT];

void intrpt_init(void) {
	size_t i;
	/* setup the idt-pointer */
	sIDTPtr idtPtr;
	idtPtr.address = (uintptr_t)idt;
	idtPtr.size = sizeof(idt) - 1;

	/* setup the idt */

	/* exceptions */
	intrpt_setIDT(0,isr0,IDT_DPL_KERNEL);
	intrpt_setIDT(1,isr1,IDT_DPL_KERNEL);
	intrpt_setIDT(2,isr2,IDT_DPL_KERNEL);
	intrpt_setIDT(3,isr3,IDT_DPL_KERNEL);
	intrpt_setIDT(4,isr4,IDT_DPL_KERNEL);
	intrpt_setIDT(5,isr5,IDT_DPL_KERNEL);
	intrpt_setIDT(6,isr6,IDT_DPL_KERNEL);
	intrpt_setIDT(7,isr7,IDT_DPL_KERNEL);
	intrpt_setIDT(8,isr8,IDT_DPL_KERNEL);
	intrpt_setIDT(9,isr9,IDT_DPL_KERNEL);
	intrpt_setIDT(10,isr10,IDT_DPL_KERNEL);
	intrpt_setIDT(11,isr11,IDT_DPL_KERNEL);
	intrpt_setIDT(12,isr12,IDT_DPL_KERNEL);
	intrpt_setIDT(13,isr13,IDT_DPL_KERNEL);
	intrpt_setIDT(14,isr14,IDT_DPL_KERNEL);
	intrpt_setIDT(15,isr15,IDT_DPL_KERNEL);
	intrpt_setIDT(16,isr16,IDT_DPL_KERNEL);
	intrpt_setIDT(17,isr17,IDT_DPL_KERNEL);
	intrpt_setIDT(18,isr18,IDT_DPL_KERNEL);
	intrpt_setIDT(19,isr19,IDT_DPL_KERNEL);
	intrpt_setIDT(20,isr20,IDT_DPL_KERNEL);
	intrpt_setIDT(21,isr21,IDT_DPL_KERNEL);
	intrpt_setIDT(22,isr22,IDT_DPL_KERNEL);
	intrpt_setIDT(23,isr23,IDT_DPL_KERNEL);
	intrpt_setIDT(24,isr24,IDT_DPL_KERNEL);
	intrpt_setIDT(25,isr25,IDT_DPL_KERNEL);
	intrpt_setIDT(26,isr26,IDT_DPL_KERNEL);
	intrpt_setIDT(27,isr27,IDT_DPL_KERNEL);
	intrpt_setIDT(28,isr28,IDT_DPL_KERNEL);
	intrpt_setIDT(29,isr29,IDT_DPL_KERNEL);
	intrpt_setIDT(30,isr30,IDT_DPL_KERNEL);
	intrpt_setIDT(31,isr31,IDT_DPL_KERNEL);
	intrpt_setIDT(32,isr32,IDT_DPL_KERNEL);

	/* hardware-interrupts */
	intrpt_setIDT(33,isr33,IDT_DPL_KERNEL);
	intrpt_setIDT(34,isr34,IDT_DPL_KERNEL);
	intrpt_setIDT(35,isr35,IDT_DPL_KERNEL);
	intrpt_setIDT(36,isr36,IDT_DPL_KERNEL);
	intrpt_setIDT(37,isr37,IDT_DPL_KERNEL);
	intrpt_setIDT(38,isr38,IDT_DPL_KERNEL);
	intrpt_setIDT(39,isr39,IDT_DPL_KERNEL);
	intrpt_setIDT(40,isr40,IDT_DPL_KERNEL);
	intrpt_setIDT(41,isr41,IDT_DPL_KERNEL);
	intrpt_setIDT(42,isr42,IDT_DPL_KERNEL);
	intrpt_setIDT(43,isr43,IDT_DPL_KERNEL);
	intrpt_setIDT(44,isr44,IDT_DPL_KERNEL);
	intrpt_setIDT(45,isr45,IDT_DPL_KERNEL);
	intrpt_setIDT(46,isr46,IDT_DPL_KERNEL);
	intrpt_setIDT(47,isr47,IDT_DPL_KERNEL);

	/* syscall */
	intrpt_setIDT(48,isr48,IDT_DPL_USER);

	/* all other interrupts */
	for(i = 49; i < 256; i++)
		intrpt_setIDT(i,isrNull,IDT_DPL_KERNEL);

	/* now we can use our idt */
	intrpt_loadidt(&idtPtr);

	/* now init the PIC */
	intrpt_initPic();
}

size_t intrpt_getCount(void) {
	return intrptCount;
}

sIntrptStackFrame *intrpt_getCurStack(void) {
	return curIntrptStack;
}

void intrpt_handler(sIntrptStackFrame *stack) {
	uint64_t cycles;
	sThread *t = thread_getRunning();
	sInterrupt *intrpt = intrptList + stack->intrptNo;
	curIntrptStack = stack;
	intrptCount++;

	if(t->tid == IDLE_TID || stack->eip < KERNEL_AREA_V_ADDR) {
		cycles = cpu_rdtsc();
		if(t->stats.ucycleStart > 0)
			t->stats.ucycleCount.val64 += cycles - t->stats.ucycleStart;
		/* kernel-mode starts here */
		t->stats.kcycleStart = cycles;
	}

	/* we need to save the page-fault address here because swapping may cause other ones */
	if(stack->intrptNo == EX_PAGE_FAULT)
		pfaddr = cpu_getCR2();

	swap_check();

	if(intrpt->handler)
		intrpt->handler(stack);
	else {
		vid_printf("Got interrupt %d (%s) @ 0x%x in process %d (%s)\n",stack->intrptNo,
				intrpt->name,stack->eip,t->proc->pid,t->proc->command);
	}

	/* handle signal */
	uenv_handleSignal();
	/* don't try to deliver the signal if we're idling currently */
	if(t->tid != IDLE_TID && uenv_hasSignalToStart())
		uenv_startSignalHandler(stack);

	/* kernel-mode ends */
	if(t->tid == IDLE_TID || stack->eip < KERNEL_AREA_V_ADDR) {
		t = thread_getRunning();
		cycles = cpu_rdtsc();
		if(t->stats.kcycleStart > 0)
			t->stats.kcycleCount.val64 += cycles - t->stats.kcycleStart;
		/* user-mode starts here */
		t->stats.ucycleStart = cycles;
	}
}

static void intrpt_exFatal(sIntrptStackFrame *stack) {
	/* count consecutive occurrences */
	if(lastEx == stack->intrptNo) {
		exCount++;

		/* stop here? */
		if(exCount >= MAX_EX_COUNT) {
			/* for exceptions in kernel: ensure that we have the default print-function */
			if(stack->eip >= KERNEL_AREA_V_ADDR)
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

static void intrpt_exGenProtFault(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	/* for exceptions in kernel: ensure that we have the default print-function */
	if(stack->eip >= KERNEL_AREA_V_ADDR)
		vid_unsetPrintFunc();
	/* io-map not loaded yet? */
	if(t->proc->ioMap != NULL && !tss_ioMapPresent()) {
		/* load it and give the process another try */
		tss_setIOMap(t->proc->ioMap);
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

static void intrpt_exCoProcNA(sIntrptStackFrame *stack) {
	UNUSED(stack);
	sThread *t = thread_getRunning();
	fpu_handleCoProcNA(&t->archAttr.fpuState);
}

static void intrpt_exPageFault(sIntrptStackFrame *stack) {
	/* for exceptions in kernel: ensure that we have the default print-function */
	if(stack->eip >= KERNEL_AREA_V_ADDR)
		vid_unsetPrintFunc();

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
			stack->eip,proc_getRunning()->pid);
#endif

	/* first let the vmm try to handle the page-fault (demand-loading, cow, swapping, ...) */
	if(!vmm_pagefault(pfaddr)) {
		/* ok, now lets check if the thread wants more stack-pages */
		if(thread_extendStack(pfaddr) < 0) {
			vid_printf("Page fault for address=0x%08x @ 0x%x, process %d\n",pfaddr,
								stack->eip,proc_getRunning()->pid);
			vid_printf("Occurred because:\n\t%s\n\t%s\n\t%s\n\t%s%s\n",
					(stack->errorCode & 0x1) ?
						"page-level protection violation" : "not-present page",
					(stack->errorCode & 0x2) ? "write" : "read",
					(stack->errorCode & 0x4) ? "user-mode" : "kernel-mode",
					(stack->errorCode & 0x8) ? "reserved bits set to 1\n\t" : "",
					(stack->errorCode & 0x16) ? "instruction-fetch" : "");

			/*proc_terminate(t->proc);
			thread_switch();*/
			/* hm...there is something wrong :) */
			/* TODO later the process should be killed here */
			util_panic("Page fault for address=0x%08x @ 0x%x",pfaddr,stack->eip);
		}
	}
}

static void intrpt_irqTimer(sIntrptStackFrame *stack) {
	sInterrupt *intrpt = intrptList + stack->intrptNo;
	if(intrpt->signal)
		sig_addSignal(intrpt->signal);
	intrpt_eoi(stack->intrptNo);
	timer_intrpt();
}

static void intrpt_irqIgnore(sIntrptStackFrame *stack) {
	sInterrupt *intrpt = intrptList + stack->intrptNo;
	if(intrpt->signal)
		sig_addSignal(intrpt->signal);
	intrpt_eoi(stack->intrptNo);
#if DEBUGGING
	/* in debug-mode, start the logviewer when the keyboard is not present yet */
	/* (with a present keyboard-driver we would steal him the scancodes) */
	/* this way, we can debug the system in the startup-phase without affecting timings
	 * (before viewing the log ;)) */
	if(stack->intrptNo == IRQ_KEYBOARD && proc_getByPid(KEYBOARD_PID) == NULL) {
		sKeyEvent ev;
		if(kb_get(&ev,KEV_PRESS,false) && ev.keycode == VK_F12)
			cons_start();
	}
#endif
}

static void intrpt_syscall(sIntrptStackFrame *stack) {
	uint argCount,ebxSave,sysCallNo;
	sThread *t = thread_getRunning();
	if(t->proc->flags & P_VM86)
		util_panic("VM86-task wants to perform a syscall!?");
	t->stats.syscalls++;

	sysCallNo = SYSC_NUMBER(stack);
	argCount = sysc_getArgCount(sysCallNo);
	ebxSave = stack->ebx;
	/* handle copy-on-write (the first 2 args are passed in registers) */
	if(sysc_getArgCount(sysCallNo) > 2) {
		/* if the arguments are not mapped, return an error */
		if(!paging_isRangeUserWritable((uintptr_t)stack->uesp,sizeof(uint) * (argCount - 2)))
			SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}
	sysc_handle(stack);

	/* set error-code (not for ackSignal) */
	if(sysCallNo != SYSCALL_ACKSIG) {
		stack->ecx = stack->ebx;
		stack->ebx = ebxSave;
	}
}

static void intrpt_initPic(void) {
	/* starts the initialization. we want to send a ICW4 */
	util_outByte(PIC_MASTER_CMD,ICW1_INIT | ICW1_NEED_ICW4);
	util_outByte(PIC_SLAVE_CMD,ICW1_INIT | ICW1_NEED_ICW4);
	/* remap the irqs to 0x20 and 0x28 */
	util_outByte(PIC_MASTER_DATA,IRQ_MASTER_BASE);
	util_outByte(PIC_SLAVE_DATA,IRQ_SLAVE_BASE);
	/* continue */
	util_outByte(PIC_MASTER_DATA,4);
	util_outByte(PIC_SLAVE_DATA,2);

	/* we want to use 8086 mode */
	util_outByte(PIC_MASTER_DATA,ICW4_8086);
	util_outByte(PIC_SLAVE_DATA,ICW4_8086);

	/* enable all interrupts (set masks to 0) */
	util_outByte(PIC_MASTER_DATA,0x00);
	util_outByte(PIC_SLAVE_DATA,0x00);
}

static void intrpt_setIDT(size_t number,fISR handler,uint8_t dpl) {
	idt[number].fix = 0xE00;
	idt[number].dpl = dpl;
	idt[number].present = number != IDT_INTEL_RES1 && number != IDT_INTEL_RES2;
	idt[number].selector = IDT_CODE_SEL;
	idt[number].offsetHigh = ((uintptr_t)handler >> 16) & 0xFFFF;
	idt[number].offsetLow = (uintptr_t)handler & 0xFFFF;
}

static void intrpt_eoi(uint32_t intrptNo) {
	/* do we have to send EOI? */
	if(intrptNo >= IRQ_MASTER_BASE && intrptNo <= IRQ_MASTER_BASE + IRQ_NUM) {
	    if(intrptNo >= IRQ_SLAVE_BASE) {
	    	/* notify the slave */
	        util_outByte(PIC_SLAVE_CMD,PIC_EOI);
	    }

	    /* notify the master */
	    util_outByte(PIC_MASTER_CMD,PIC_EOI);
    }
}

#if DEBUGGING

void intrpt_dbg_printStackFrame(const sIntrptStackFrame *stack) {
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
