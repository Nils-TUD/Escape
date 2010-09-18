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
#include <sys/machine/intrpt.h>
#include <sys/machine/cpu.h>
#include <sys/task/proc.h>
#include <sys/mem/pmem.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/mem/vmm.h>
#include <sys/ksymbols.h>
#include <sys/video.h>
#include <sys/util.h>
#include <sys/log.h>
#include <esc/register.h>
#include <stdarg.h>
#include <string.h>

/* for reading from kb */
#define IOPORT_KB_DATA				0x60
#define IOPORT_KB_CTRL				0x64
#define STATUS_OUTBUF_FULL			(1 << 0)

#define VIDEO_BASE					0xC00B8000

/* the x86-call instruction is 5 bytes long */
#define CALL_INSTR_SIZE			5

static u8 util_waitForKey(void);

#if DEBUGGING
static char buffer[512];
static char backup[VID_ROWS * VID_COLS * 2];
#endif

/* the beginning of the kernel-stack */
extern u32 kernelStack;
static u64 profStart;

/* source: http://en.wikipedia.org/wiki/Linear_congruential_generator */
static u32 randa = 1103515245;
static u32 randc = 12345;
static u32 lastRand = 0;

void util_panic(const char *fmt,...) {
	static u32 regs[REG_COUNT];
	sIntrptStackFrame *istack = intrpt_getCurStack();
	sThread *t = thread_getRunning();
	va_list ap;

	/* enter video-mode 0x2 to be sure that the user can see the panic :) */
	/* actually it may fail depending on what caused the panic. this may make it more difficult
	 * to find the real reason for a failure. so it might be a good idea to turn it off during
	 * kernel-debugging :)  */
	sVM86Regs vmregs;
	memset(&regs,0,sizeof(regs));
	vmregs.ax = 0x2;
	vm86_int(0x10,&vmregs,NULL,0);
	vid_clearScreen();

	/* disable interrupts so that nothing fancy can happen */
	intrpt_setEnabled(false);

	/* print message */
	vid_printf("\n");
	vid_printf("\033[co;7;4]PANIC: ");
	va_start(ap,fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);
	vid_printf("%|s\033[co]\n","");

	if(t != NULL)
		vid_printf("Caused by thread %d (%s)\n\n",t->tid,t->proc->command);
	util_printStackTrace(util_getKernelStackTrace());

	if(t != NULL && t->stackRegion) {
		util_printStackTrace(util_getUserStackTrace());
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

#if DEBUGGING
	/* write into log only */
	vid_setTargets(TARGET_SCREEN);
	vid_printf("\n\nWriting regions and page-directory of the current process to log...");
	vid_setTargets(TARGET_LOG);
	vmm_dbg_print(t->proc);
	paging_dbg_printCur(PD_PART_USER);
	vid_setTargets(TARGET_SCREEN);
	vid_printf("Done\n\nPress any key to view log");
	while(1) {
		util_waitForKeyPress();
		util_logViewer();
	}
#else
	/* TODO vmware seems to shutdown if we disable interrupts and htl?? */
	while(1)
		util_halt();
#endif
}

void util_logViewer(void) {
	tInodeNo nodeNo;
	tFileNo file;
	u8 scanCode;
	s32 i,res,start,end,lineCount = 0,linePos = 0,lineSize = 16;
	char **lines = (char**)kheap_alloc(lineSize * sizeof(char*));
	if(!lines)
		goto error;
	lines[lineCount] = kheap_alloc(VID_COLS + 1);
	if(!lines[lineCount])
		goto error;

	/* make a screen-backup */
	memcpy(backup,(void*)VIDEO_BASE,VID_ROWS * VID_COLS * 2);

	/* don't write the following to the log ;) */
	vid_setTargets(TARGET_SCREEN);
	res = vfsn_resolvePath("/system/log",&nodeNo,NULL,VFS_READ);
	if(res < 0)
		goto error;
	file = vfs_openFile(KERNEL_TID,VFS_READ,nodeNo,VFS_DEV_NO);
	if(file < 0)
		goto error;
	while((res = vfs_readFile(KERNEL_TID,file,(u8*)buffer,sizeof(buffer))) > 0) {
		/* build lines from the read data */
		for(i = 0; i < res; i++) {
			if(buffer[i] == '\n') {
				/* fill up with spaces */
				for(; linePos < VID_COLS; linePos++)
					lines[lineCount][linePos] = ' ';
				lines[lineCount][linePos] = '\0';
				/* to next line */
				linePos = 0;
				lineCount++;
				/* allocate more lines if necessary */
				if(lineCount >= lineSize) {
					lineSize *= 2;
					lines = (char**)kheap_realloc(lines,lineSize * sizeof(char*));
					if(!lines)
						goto error;
				}
				/* allocate new line */
				lines[lineCount] = kheap_alloc(VID_COLS + 1);
				if(!lines[lineCount])
					goto error;
			}
			/* append */
			else if(linePos < VID_COLS)
				lines[lineCount][linePos++] = buffer[i];
		}
	}
	/* end last line */
	for(; linePos < VID_COLS; linePos++)
		lines[lineCount][linePos] = ' ';
	lines[lineCount][linePos] = '\0';
	lineCount++;
	vfs_closeFile(KERNEL_TID,file);

	/* start viewing */
	start = 0;
	while(true) {
		/* print lines */
		vid_clearScreen();
		for(i = start, end = MIN(lineCount,start + VID_ROWS); i < end; i++)
			vid_printf("%s\r",lines[i]);
		for(; (i - start) < VID_ROWS; i++)
			vid_printf("\n");
		/* print info-line */
		vid_printf("\033[co;0;7]LogViewer%|s\033[co]",
				"Navigation: a=up, y/z=down, s=page up, x=page down, q=quit");
		/* wait for key */
		scanCode = util_waitForKeyPress();
		if(scanCode == 0x1E && start > 0)					/* a */
			start--;
		else if(scanCode == 0x2C && start < lineCount - VID_ROWS - 1)	/* y/z */
			start++;
		else if(scanCode == 0x1F)							/* s */
			start = MAX(start - (VID_ROWS - 1),0);
		else if(scanCode == 0x2D)							/* x */
			start = MIN(start + (VID_ROWS - 1),lineCount - VID_ROWS - 1);
		else if(scanCode == 0x10)							/* q */
			break;
	}

	/* clean up */
	for(i = 0; i < lineCount; i++)
		kheap_free(lines[i]);
	kheap_free(lines);
	vid_setTargets(TARGET_SCREEN | TARGET_LOG);

	/* restore screen-backup */
	memcpy((void*)VIDEO_BASE,backup,VID_ROWS * VID_COLS * 2);
	return;

error:
	vid_printf("Viewing log failed\n");
}

s32 util_rand(void) {
	s32 res;
	lastRand = randa * lastRand + randc;
	res = (s32)((u32)(lastRand / 65536) % 32768);
	return res;
}

void util_srand(u32 seed) {
	lastRand = seed;
}

u8 util_getScanCode(void) {
	if(util_inByte(IOPORT_KB_CTRL) & STATUS_OUTBUF_FULL)
		return util_inByte(IOPORT_KB_DATA);
	return 0;
}

u8 util_waitForKeyPress(void) {
	u8 scanCode;
	while(1) {
		scanCode = util_waitForKey();
		if(!(scanCode & 0x80))
			break;
	}
	return scanCode;
}

u8 util_waitForKeyRelease(void) {
	u8 scanCode;
	while(1) {
		scanCode = util_waitForKey();
		if(scanCode & 0x80)
			break;
	}
	return scanCode & ~0x80;
}

static u8 util_waitForKey(void) {
	while(!(util_inByte(IOPORT_KB_CTRL) & STATUS_OUTBUF_FULL))
		;
	return util_inByte(IOPORT_KB_DATA);
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
	u32 start,end;
	sIntrptStackFrame *stack = intrpt_getCurStack();
	sThread *t = thread_getRunning();
	vmm_getRegRange(t->proc,t->stackRegion,&start,&end);
	return util_getStackTrace((u32*)stack->ebp,start,end,start,end);
}

sFuncCall *util_getUserStackTraceOf(sThread *t) {
	u32 start,end,pcount;
	sFuncCall *calls;
	u32 *frames;
	if(t->stackRegion >= 0) {
		vmm_getRegRange(t->proc,t->stackRegion,&start,&end);
		pcount = (end - start) / PAGE_SIZE;
		frames = kheap_alloc((pcount + 2) * sizeof(u32));
		if(frames) {
			sIntrptStackFrame *istack = intrpt_getCurStack();
			u32 temp,i,startCpy = start;
			frames[0] = t->kstackFrame;
			for(i = 0; startCpy < end; i++) {
				if(!paging_isPresent(t->proc->pagedir,startCpy)) {
					kheap_free(frames);
					return NULL;
				}
				frames[i + 1] = paging_getFrameNo(t->proc->pagedir,startCpy);
				startCpy += PAGE_SIZE;
			}
			temp = paging_mapToTemp(frames,pcount + 1);
			istack = (sIntrptStackFrame*)(temp + ((u32)istack & (PAGE_SIZE - 1)));
			calls = util_getStackTrace((u32*)istack->ebp,start,end,
					temp + PAGE_SIZE,temp + (pcount + 1) * PAGE_SIZE);
			paging_unmapFromTemp(pcount + 1);
			kheap_free(frames);
			return calls;
		}
	}
	return NULL;
}

sFuncCall *util_getKernelStackTraceOf(sThread *t) {
	u32 ebp = t->save.ebp;
	u32 temp = paging_mapToTemp(&t->kstackFrame,1);
	sFuncCall *calls = util_getStackTrace((u32*)ebp,KERNEL_STACK,KERNEL_STACK + PAGE_SIZE,
			temp,temp + PAGE_SIZE);
	paging_unmapFromTemp(1);
	return calls;
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

	return util_getStackTrace(ebp,start,end,start,end);
}

sFuncCall *util_getStackTrace(u32 *ebp,u32 rstart,u32 rend,u32 mstart,u32 mend) {
	static sFuncCall frames[MAX_STACK_DEPTH];
	u32 i;
	bool isKernel = (u32)ebp >= KERNEL_AREA_V_ADDR;
	sFuncCall *frame = &frames[0];
	sSymbol *sym;

	for(i = 0; i < MAX_STACK_DEPTH; i++) {
		if(ebp == NULL)
			break;
		/* adjust it if we're in the kernel-stack but are using the temp-area (to print the trace
		 * for another thread). don't do this for the temp-kernel-stack */
		if(rstart != ((u32)&kernelStack) - TMP_STACK_SIZE && rstart != mstart)
			ebp = (u32*)(mstart + ((u32)ebp & (PAGE_SIZE - 1)));
		/* prevent page-fault */
		if((u32)ebp < mstart || (u32)ebp >= mend)
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
		if(i % 16 == 15)
			vid_printf("\n");
	}
}
