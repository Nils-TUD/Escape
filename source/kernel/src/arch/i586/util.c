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
#include <sys/vfs/vfs.h>
#include <sys/cpu.h>
#include <sys/intrpt.h>
#include <sys/ksymbols.h>
#include <sys/video.h>
#include <sys/util.h>
#include <sys/log.h>
#include <esc/keycodes.h>
#include <esc/messages.h>
#include <errors.h>
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

	/* enter vga-mode to be sure that the user can see the panic :) */
	/* actually it may fail depending on what caused the panic. this may make it more difficult
	 * to find the real reason for a failure. so it might be a good idea to turn it off during
	 * kernel-debugging :) */
	file_t file = vfs_openPath(KERNEL_PID,VFS_MSGS,"/dev/video");
	if(file >= 0) {
		ssize_t res;
		vfs_sendMsg(KERNEL_PID,file,MSG_VID_SETMODE,NULL,0);
		do {
			res = vfs_receiveMsg(KERNEL_PID,file,NULL,NULL,0);
		}
		while(res == ERR_INTERRUPTED);
		vfs_closeFile(KERNEL_PID,file);
	}
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
		sIntrptStackFrame *kstack = thread_getIntrptStack(t);
		util_printStackTrace(util_getUserStackTrace());
		vid_printf("User-Register:\n");
		regs[R_EAX] = kstack->eax;
		regs[R_EBX] = kstack->ebx;
		regs[R_ECX] = kstack->ecx;
		regs[R_EDX] = kstack->edx;
		regs[R_ESI] = kstack->esi;
		regs[R_EDI] = kstack->edi;
		regs[R_ESP] = kstack->uesp;
		regs[R_EBP] = kstack->ebp;
		regs[R_CS] = kstack->cs;
		regs[R_DS] = kstack->ds;
		regs[R_ES] = kstack->es;
		regs[R_FS] = kstack->fs;
		regs[R_GS] = kstack->gs;
		regs[R_SS] = kstack->uss;
		regs[R_EFLAGS] = kstack->eflags;
		PRINT_REGS(regs,"\t");
	}

	/* write into log only */
	vid_setTargets(TARGET_SCREEN);
	vid_printf("\n\nWriting regions and page-directory of the current process to log...");
	vid_setTargets(TARGET_LOG);
	vmm_print(t->proc->pid);
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
	sThread *t = thread_getRunning();
	sIntrptStackFrame *kstack = thread_getIntrptStack(t);
	if(thread_getStackRange(t,&start,&end,0))
		return util_getStackTrace((uint32_t*)kstack->ebp,start,start,end);
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

sFuncCall *util_getUserStackTraceOf(sThread *t) {
	uintptr_t start,end;
	size_t pcount;
	sFuncCall *calls;
	frameno_t *frames;
	if(thread_getStackRange(t,&start,&end,0)) {
		pcount = (end - start) / PAGE_SIZE;
		frames = cache_alloc((pcount + 2) * sizeof(frameno_t));
		if(frames) {
			sIntrptStackFrame *istack = thread_getIntrptStack(t);
			uintptr_t temp,startCpy = start;
			size_t i;
			frames[0] = t->kstackFrame;
			for(i = 0; startCpy < end; i++) {
				if(!paging_isPresent(&t->proc->pagedir,startCpy)) {
					cache_free(frames);
					return NULL;
				}
				frames[i + 1] = paging_getFrameNo(&t->proc->pagedir,startCpy);
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
	/* for the current, we can't use the ebp from the context-switch. instead we have to use the
	 * value on the interrupt-stack */
	uint32_t ebp = t == thread_getRunning() ? thread_getIntrptStack(t)->ebp : t->save.ebp;
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

