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
#include <sys/task/smp.h>
#include <sys/mem/physmem.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/cache.h>
#include <sys/mem/virtmem.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/openfile.h>
#include <sys/cpu.h>
#include <sys/interrupts.h>
#include <sys/ksymbols.h>
#include <sys/video.h>
#include <sys/util.h>
#include <sys/log.h>
#include <esc/keycodes.h>
#include <esc/messages.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

/* the x86-call instruction is 5 bytes long */
#define CALL_INSTR_SIZE			5

static Util::FuncCall *getStackTrace(uint32_t *ebp,uintptr_t rstart,uintptr_t mstart,uintptr_t mend);

/* the beginning of the kernel-stack */
extern uintptr_t kernelStack;

void Util::panicArch() {
	/* at first, halt the other CPUs */
	SMP::haltOthers();

	/* enter vga-mode to be sure that the user can see the panic :) */
	/* actually it may fail depending on what caused the panic. this may make it more difficult
	 * to find the real reason for a failure. so it might be a good idea to turn it off during
	 * kernel-debugging :) */
	switchToVGA();
}

void Util::switchToVGA() {
	OpenFile *file;
	if(VFS::openPath(KERNEL_PID,VFS_MSGS | VFS_NOBLOCK,"/dev/vga",&file) == 0) {
		ssize_t res;
		sStrMsg msg;
		msg.arg1 = 3;
		msg.arg2 = VID_MODE_TYPE_TUI;
		msg.arg3 = true;
		/* use an empty shm-name here. we don't need that anyway */
		msg.s1[0] = '\0';
		res = file->sendMsg(KERNEL_PID,MSG_SCR_SETMODE,&msg,sizeof(msg),NULL,0);
		if(res >= 0) {
			for(int i = 0; i < 100; i++) {
				res = file->receiveMsg(KERNEL_PID,NULL,NULL,0,false);
				if(res >= 0)
					break;
				Thread::switchAway();
			}
		}
		file->close(KERNEL_PID);
	}
}

void Util::printUserStateOf(OStream &os,const Thread *t) {
	if(t->getIntrptStack()) {
		frameno_t frame = t->getProc()->getPageDir()->getFrameNo(t->getKernelStack());
		uintptr_t kstackAddr = PageDir::mapToTemp(&frame,1);
		size_t kstackOff = (uintptr_t)t->getIntrptStack() & (PAGE_SIZE - 1);
		IntrptStackFrame *kstack = (IntrptStackFrame*)(kstackAddr + kstackOff);
		os.writef("User-Register:\n");
		os.writef("\teax=%#08x, ebx=%#08x, ecx=%#08x, edx=%#08x\n",
				kstack->eax,kstack->ebx,kstack->ecx,kstack->edx);
		os.writef("\tesi=%#08x, edi=%#08x, esp=%#08x, ebp=%#08x\n",
				kstack->esi,kstack->edi,kstack->getSP(),kstack->ebp);
		os.writef("\teip=%#08x, eflags=%#08x\n",kstack->getIP(),kstack->getFlags());
		os.writef("\tcr0=%#08x, cr2=%#08x, cr3=%#08x, cr4=%#08x\n",
				CPU::getCR0(),CPU::getCR2(),CPU::getCR3(),CPU::getCR4());
		PageDir::unmapFromTemp(1);
	}
}

void Util::printUserState(OStream &os) {
	const Thread *t = Thread::getRunning();
	printUserStateOf(os,t);
}

Util::FuncCall *Util::getUserStackTrace() {
	Thread *t = Thread::getRunning();
	IntrptStackFrame *kstack = t->getIntrptStack();
	if(kstack) {
		uintptr_t start,end;
		if(t->getStackRange(&start,&end,0))
			return getStackTrace((uint32_t*)kstack->ebp,start,start,end);
	}
	return NULL;
}

Util::FuncCall *Util::getKernelStackTrace() {
	uint32_t* ebp;
	Thread *t = Thread::getRunning();
	if(t) {
		asm volatile ("mov %%ebp,%0" : "=a" (ebp) : );

		/* determine the stack-bounds; we have a temp stack at the beginning */
		uintptr_t start,end;
		if((uintptr_t)ebp >= t->getKernelStack() &&
				(uintptr_t)ebp < t->getKernelStack() + PAGE_SIZE) {
			start = t->getKernelStack();
			end = t->getKernelStack() + PAGE_SIZE;
		}
		else {
			start = ((uintptr_t)&kernelStack) - TMP_STACK_SIZE;
			end = (uintptr_t)&kernelStack;
		}
		return getStackTrace(ebp,start,start,end);
	}
	return NULL;
}

Util::FuncCall *Util::getUserStackTraceOf(Thread *t) {
	uintptr_t start,end;
	if(t->getStackRange(&start,&end,0)) {
		PageDir *pdir = t->getProc()->getPageDir();
		size_t pcount = (end - start) / PAGE_SIZE;
		frameno_t *frames = (frameno_t*)Cache::alloc((pcount + 2) * sizeof(frameno_t));
		if(frames) {
			IntrptStackFrame *istack = t->getIntrptStack();
			uintptr_t temp,startCpy = start;
			frames[0] = pdir->getFrameNo(t->getKernelStack());
			for(size_t i = 0; startCpy < end; i++) {
				if(!pdir->isPresent(startCpy)) {
					Cache::free(frames);
					return NULL;
				}
				frames[i + 1] = pdir->getFrameNo(startCpy);
				startCpy += PAGE_SIZE;
			}
			temp = PageDir::mapToTemp(frames,pcount + 1);
			istack = (IntrptStackFrame*)(temp + ((uintptr_t)istack & (PAGE_SIZE - 1)));
			FuncCall *calls = getStackTrace((uint32_t*)istack->ebp,start,
					temp + PAGE_SIZE,temp + (pcount + 1) * PAGE_SIZE);
			PageDir::unmapFromTemp(pcount + 1);
			Cache::free(frames);
			return calls;
		}
	}
	return NULL;
}

Util::FuncCall *Util::getKernelStackTraceOf(const Thread *t) {
	Thread *run = Thread::getRunning();
	if(run == t)
		return getKernelStackTrace();
	else {
		uint32_t ebp = t->getRegs().ebp;
		frameno_t frame = t->getProc()->getPageDir()->getFrameNo(t->getKernelStack());
		uintptr_t temp = PageDir::mapToTemp(&frame,1);
		FuncCall *calls = getStackTrace((uint32_t*)ebp,t->getKernelStack(),temp,temp + PAGE_SIZE);
		PageDir::unmapFromTemp(1);
		return calls;
	}
}

Util::FuncCall *getStackTrace(uint32_t *ebp,uintptr_t rstart,uintptr_t mstart,uintptr_t mend) {
	static Util::FuncCall frames[Util::MAX_STACK_DEPTH];
	bool isKernel = (uintptr_t)ebp >= KERNEL_AREA;
	Util::FuncCall *frame = &frames[0];

	for(size_t i = 0; i < Util::MAX_STACK_DEPTH; i++) {
		if(ebp == NULL)
			break;
		/* adjust it if we're in the kernel-stack but are using the temp-area (to print the trace
		 * for another thread). don't do this for the temp-kernel-stack */
		if(rstart != ((uintptr_t)&kernelStack) - TMP_STACK_SIZE && rstart != mstart)
			ebp = (uint32_t*)(mstart + ((uintptr_t)ebp & (PAGE_SIZE - 1)));
		/* prevent page-fault */
		if((uintptr_t)ebp < mstart || (uintptr_t)(ebp + 1) < mstart ||
				(((uintptr_t)(ebp + 1) + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1)) >= mend)
			break;
		frame->addr = *(ebp + 1) - CALL_INSTR_SIZE;
		KSymbols::Symbol *sym = isKernel ? KSymbols::getSymbolAt(frame->addr) : NULL;
		if(sym) {
			frame->funcAddr = sym->address;
			frame->funcName = sym->funcName;
		}
		else {
			frame->funcAddr = frame->addr;
			frame->funcName = "Unknown";
		}
		frame++;
		/* detect loops */
		uint32_t *oldebp = ebp;
		ebp = (uint32_t*)*ebp;
		if(ebp == oldebp)
			break;
	}

	/* terminate */
	frame->addr = 0;
	return &frames[0];
}

