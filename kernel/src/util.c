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

#include <common.h>
#include <machine/intrpt.h>
#include <task/proc.h>
#include <mem/pmem.h>
#include <mem/paging.h>
#include <mem/kheap.h>
#include <ksymbols.h>
#include <video.h>
#include <stdarg.h>
#include <string.h>
#include <register.h>
#include <util.h>

/* the x86-call instruction is 5 bytes long */
#define CALL_INSTR_SIZE			5

/* the beginning of the kernel-stack */
extern u32 kernelStack;

void util_panic(const char *fmt,...) {
	u32 regs[REG_COUNT];
	sIntrptStackFrame *istack = intrpt_getCurStack();
	sThread *t = thread_getRunning();
	va_list ap;
	vid_printf("\n");
	vid_setLineBG(vid_getLine(),RED);
	vid_useColor(RED,WHITE);
	vid_printf("PANIC: ");

	/* print message */
	va_start(ap,fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);

	vid_printf("\n");
	vid_restoreColor();
	if(t != NULL)
		vid_printf("Caused by thread %d (%s)\n\n",t->tid,t->proc->command);
	util_printStackTrace(util_getKernelStackTrace());

	if(t != NULL) {
		util_printStackTrace(util_getUserStackTrace(t,intrpt_getCurStack()));
		vid_printf("User-Register:\n");
		regs[R_EAX] = istack->eax;
		regs[R_EBX] = istack->ebx;
		regs[R_ECX] = istack->ecx;
		regs[R_EDX] = istack->edx;
		regs[R_ESI] = istack->esi;
		regs[R_EDI] = istack->edi;
		regs[R_ESP] = istack->uesp;
		regs[R_EBP] = istack->ebp;
		regs[R_CS] = istack->cs;
		regs[R_DS] = istack->ds;
		regs[R_ES] = istack->es;
		regs[R_FS] = istack->fs;
		regs[R_GS] = istack->gs;
		regs[R_SS] = istack->uss;
		regs[R_EFLAGS] = istack->eflags;
		PRINT_REGS(regs,"\t");
	}

	/*proc_dbg_printAll();*/
	intrpt_setEnabled(false);
	util_halt();
}

sFuncCall *util_getUserStackTrace(sThread *t,sIntrptStackFrame *stack) {
	return util_getStackTrace((u32*)stack->ebp,
			t->ustackBegin - t->ustackPages * PAGE_SIZE,t->ustackBegin);
}

sFuncCall *util_getKernelStackTrace(void) {
	u32 start,end;
	u32* ebp = (u32*)getStackFrameStart();

	/* determine the stack-bounds; we have a temp stack at the beginning */
	if((u32)ebp >= KERNEL_STACK && (u32)ebp < KERNEL_STACK + PAGE_SIZE) {
		start = KERNEL_STACK;
		end = KERNEL_STACK + PAGE_SIZE;
	}
	else {
		start = ((u32)&kernelStack) - TMP_STACK_SIZE;
		end = (u32)&kernelStack;
	}

	return util_getStackTrace(ebp,start,end);
}

sFuncCall *util_getStackTrace(u32 *ebp,u32 start,u32 end) {
	static sFuncCall frames[MAX_STACK_DEPTH];
	u32 i;
	bool isKernel = (u32)ebp >= KERNEL_AREA_V_ADDR;
	sFuncCall *frame = &frames[0];
	sSymbol *sym;

	for(i = 0; i < MAX_STACK_DEPTH; i++) {
		/* prevent page-fault */
		if((u32)ebp < start || (u32)ebp >= end)
			break;
		frame->addr = *(ebp + 1) - CALL_INSTR_SIZE;
		if(isKernel) {
			sym = ksym_getSymbolAt(frame->addr);
			frame->funcAddr = sym->address;
			frame->funcName = sym->funcName;
		}
		else {
			frame->funcAddr = frame->addr;
			frame->funcName = "Unknown";
		}
		ebp = (u32*)*ebp;
		frame++;
	}

	/* terminate */
	frame->addr = 0;
	return &frames[0];
}

void util_printStackTrace(sFuncCall *trace) {
	if(trace->addr < KERNEL_AREA_V_ADDR)
		vid_printf("User-Stacktrace:\n");
	else
		vid_printf("Kernel-Stacktrace:\n");

	/* TODO maybe we should skip util_printStackTrace here? */
	while(trace->addr != 0) {
		vid_printf("\t0x%08x -> 0x%08x (%s)\n",(trace + 1)->addr,trace->funcAddr,trace->funcName);
		trace++;
	}
}

void util_dumpMem(void *addr,u32 dwordCount) {
	u32 *ptr = (u32*)addr;
	while(dwordCount-- > 0) {
		vid_printf("0x%x: 0x%08x\n",ptr,*ptr);
		ptr++;
	}
}

void util_dumpBytes(void *addr,u32 byteCount) {
	u32 i = 0;
	u8 *ptr = (u8*)addr;
	for(i = 0; byteCount-- > 0; i++) {
		vid_printf("%02x ",*ptr);
		ptr++;
		if(i % 12 == 11)
			vid_printf("\n");
	}
}
