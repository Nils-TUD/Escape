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
#include <esc/arch/i586/register.h>
#include <sys/arch/i586/task/vm86.h>
#include <sys/task/proc.h>
#include <sys/dbg/kb.h>
#include <sys/dbg/console.h>
#include <sys/mem/pmem.h>
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/mem/vmm.h>
#include <sys/cpu.h>
#include <sys/intrpt.h>
#include <sys/ksymbols.h>
#include <sys/video.h>
#include <sys/util.h>
#include <sys/log.h>
#include <esc/keycodes.h>
#include <stdarg.h>
#include <string.h>

/* the x86-call instruction is 5 bytes long */
#define CALL_INSTR_SIZE			5

static sFuncCall *util_getStackTrace(uint32_t *ebp,uintptr_t rstart,uintptr_t mstart,uintptr_t mend);

/* the beginning of the kernel-stack */
extern uintptr_t kernelStack;
static uint64_t profStart;

void util_panic(const char *fmt,...) {
	static uint32_t regs[REG_COUNT];
	const sThread *t = thread_getRunning();
	va_list ap;

	/* enter video-mode 0x2 to be sure that the user can see the panic :) */
	/* actually it may fail depending on what caused the panic. this may make it more difficult
	 * to find the real reason for a failure. so it might be a good idea to turn it off during
	 * kernel-debugging :) */
	/*sVM86Regs vmregs;
	memclear(&vmregs,sizeof(vmregs));
	vmregs.ax = 0x2;
	vm86_int(0x10,&vmregs,NULL,0);*/
	vid_clearScreen();

	/* disable interrupts so that nothing fancy can happen */
	intrpt_setEnabled(false);

	/* print message */
	vid_setTargets(TARGET_SCREEN | TARGET_LOG);
	vid_printf("\n");
	vid_printf("\033[co;7;4]PANIC: ");
	va_start(ap,fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);
	vid_printf("%|s\033[co]\n","");

	if(t != NULL)
		vid_printf("Caused by thread %d (%s)\n\n",t->tid,t->proc->command);
	util_printStackTrace(util_getKernelStackTrace());

	if(t != NULL) {
		util_printStackTrace(util_getUserStackTrace());
		vid_printf("User-Register:\n");
		regs[R_EAX] = t->kstackEnd->eax;
		regs[R_EBX] = t->kstackEnd->ebx;
		regs[R_ECX] = t->kstackEnd->ecx;
		regs[R_EDX] = t->kstackEnd->edx;
		regs[R_ESI] = t->kstackEnd->esi;
		regs[R_EDI] = t->kstackEnd->edi;
		regs[R_ESP] = t->kstackEnd->uesp;
		regs[R_EBP] = t->kstackEnd->ebp;
		regs[R_CS] = t->kstackEnd->cs;
		regs[R_DS] = t->kstackEnd->ds;
		regs[R_ES] = t->kstackEnd->es;
		regs[R_FS] = t->kstackEnd->fs;
		regs[R_GS] = t->kstackEnd->gs;
		regs[R_SS] = t->kstackEnd->uss;
		regs[R_EFLAGS] = t->kstackEnd->eflags;
		PRINT_REGS(regs,"\t");
	}

	/* write into log only */
	vid_setTargets(TARGET_SCREEN);
	vid_printf("\n\nWriting regions and page-directory of the current process to log...");
	vid_setTargets(TARGET_LOG);
	vmm_print(t->proc);
	paging_printCur(PD_PART_USER);
	vid_setTargets(TARGET_SCREEN);
	vid_printf("Done\n\nPress any key to start debugger");
	while(1) {
		kb_get(NULL,KEV_PRESS,true);
		cons_start();
	}
}

void util_startTimer(void) {
	profStart = cpu_rdtsc();
}

void util_stopTimer(const char *prefix,...) {
	va_list l;
	uLongLong diff;
	diff.val64 = cpu_rdtsc() - profStart;
	va_start(l,prefix);
	vid_vprintf(prefix,l);
	va_end(l);
	vid_printf(": 0x%08x%08x\n",diff.val32.upper,diff.val32.lower);
}

sFuncCall *util_getUserStackTrace(void) {
	uintptr_t start,end;
	const sThread *t = thread_getRunning();
	if(thread_getStackRange(t,&start,&end,0))
		return util_getStackTrace((uint32_t*)t->kstackEnd->ebp,start,start,end);
	return NULL;
}

sFuncCall *util_getKernelStackTrace(void) {
	uintptr_t start,end;
	uint32_t* ebp = (uint32_t*)getStackFrameStart();

	/* determine the stack-bounds; we have a temp stack at the beginning */
	if((uintptr_t)ebp >= KERNEL_STACK && (uintptr_t)ebp < KERNEL_STACK + PAGE_SIZE) {
		start = KERNEL_STACK;
		end = KERNEL_STACK + PAGE_SIZE;
	}
	else {
		start = ((uintptr_t)&kernelStack) - TMP_STACK_SIZE;
		end = (uintptr_t)&kernelStack;
	}

	return util_getStackTrace(ebp,start,start,end);
}

sFuncCall *util_getUserStackTraceOf(const sThread *t) {
	uintptr_t start,end;
	size_t pcount;
	sFuncCall *calls;
	frameno_t *frames;
	if(thread_getStackRange(t,&start,&end,0)) {
		pcount = (end - start) / PAGE_SIZE;
		frames = cache_alloc((pcount + 2) * sizeof(frameno_t));
		if(frames) {
			sIntrptStackFrame *istack = t->kstackEnd;
			uintptr_t temp,startCpy = start;
			size_t i;
			frames[0] = t->kstackFrame;
			for(i = 0; startCpy < end; i++) {
				if(!paging_isPresent(t->proc->pagedir,startCpy)) {
					cache_free(frames);
					return NULL;
				}
				frames[i + 1] = paging_getFrameNo(t->proc->pagedir,startCpy);
				startCpy += PAGE_SIZE;
			}
			temp = paging_mapToTemp(frames,pcount + 1);
			istack = (sIntrptStackFrame*)(temp + ((uintptr_t)istack & (PAGE_SIZE - 1)));
			calls = util_getStackTrace((uint32_t*)istack->ebp,start,
					temp + PAGE_SIZE,temp + (pcount + 1) * PAGE_SIZE);
			paging_unmapFromTemp(pcount + 1);
			cache_free(frames);
			return calls;
		}
	}
	return NULL;
}

sFuncCall *util_getKernelStackTraceOf(const sThread *t) {
	uint32_t ebp = t->save.ebp;
	uintptr_t temp = paging_mapToTemp(&t->kstackFrame,1);
	sFuncCall *calls = util_getStackTrace((uint32_t*)ebp,KERNEL_STACK,temp,temp + PAGE_SIZE);
	paging_unmapFromTemp(1);
	return calls;
}

static sFuncCall *util_getStackTrace(uint32_t *ebp,uintptr_t rstart,uintptr_t mstart,uintptr_t mend) {
	static sFuncCall frames[MAX_STACK_DEPTH];
	size_t i;
	bool isKernel = (uintptr_t)ebp >= KERNEL_START;
	sFuncCall *frame = &frames[0];
	sSymbol *sym;

	for(i = 0; i < MAX_STACK_DEPTH; i++) {
		if(ebp == NULL)
			break;
		/* adjust it if we're in the kernel-stack but are using the temp-area (to print the trace
		 * for another thread). don't do this for the temp-kernel-stack */
		if(rstart != ((uintptr_t)&kernelStack) - TMP_STACK_SIZE && rstart != mstart)
			ebp = (uint32_t*)(mstart + ((uintptr_t)ebp & (PAGE_SIZE - 1)));
		/* prevent page-fault */
		if((uintptr_t)ebp < mstart ||
				(((uintptr_t)(ebp + 1) + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1)) >= mend)
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
		ebp = (uint32_t*)*ebp;
		frame++;
	}

	/* terminate */
	frame->addr = 0;
	return &frames[0];
}

