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

#include <esc/ipc/ipcbuf.h>
#include <sys/messages.h>
#include <task/proc.h>
#include <task/smp.h>
#include <vfs/vfs.h>
#include <common.h>
#include <util.h>

static uintptr_t *getStackTrace(ulong *bp,uintptr_t rstart,uintptr_t mstart,uintptr_t mend);

/* the x86-call instruction is 5 bytes long */
static const int CALL_INSTR_SIZE	= 5;

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
	if(VFS::openPath(Proc::getRunning(),VFS_MSGS | VFS_NOBLOCK,0,"/dev/vga",NULL,&file) == 0) {
		ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
		esc::IPCBuf ib(buffer,sizeof(buffer));
		ib << 3 << 1 << true;

		ssize_t res = file->sendMsg(MSG_SCR_SETMODE,ib.buffer(),ib.pos(),NULL,0);
		if(res > 0) {
			for(int i = 0; i < 100; i++) {
				msgid_t mid = res;
				res = file->receiveMsg(&mid,NULL,0,VFS_NOBLOCK);
				if(res >= 0)
					break;
				Thread::switchAway();
			}
		}
		file->close();
	}
}

void Util::printUserStateOf(OStream &os,const Thread *t) {
	if(t->getUserState()) {
		frameno_t frame = t->getProc()->getPageDir()->getFrameNo(t->getKernelStack());
		uintptr_t kstackAddr = PageDir::getAccess(frame);
		size_t kstackOff = (uintptr_t)t->getUserState() & (PAGE_SIZE - 1);
		IntrptStackFrame *kstack = (IntrptStackFrame*)(kstackAddr + kstackOff);
		os.writef("User-Register:\n");
		kstack->print(os);
		os.writef("\tcr0: %#08x cr2: %#08x\n",CPU::getCR0(),CPU::getCR2());
		os.writef("\tcr3: %#08x cr4: %#08x\n",CPU::getCR3(),CPU::getCR4());
		PageDir::removeAccess(frame);
	}
}

void Util::printUserState(OStream &os) {
	const Thread *t = Thread::getRunning();
	printUserStateOf(os,t);
}

uintptr_t *Util::getUserStackTrace() {
	Thread *t = Thread::getRunning();
	IntrptStackFrame *kstack = t->getUserState();
	if(kstack) {
		uintptr_t start,end;
		if(t->getStackRange(&start,&end,0))
			return getStackTrace((ulong*)kstack->getBP(),start,start,end);
	}
	return NULL;
}

uintptr_t *Util::getKernelStackTrace() {
	Thread *t = Thread::getRunning();
	if(t) {
		ulong* bp;
		GET_REG(bp,bp);

		/* determine the stack-bounds; we have a temp stack at the beginning */
		uintptr_t start,end;
		if((uintptr_t)bp >= t->getKernelStack() &&
				(uintptr_t)bp < t->getKernelStack() + PAGE_SIZE) {
			start = t->getKernelStack();
			end = t->getKernelStack() + PAGE_SIZE;
		}
		else {
			start = ((uintptr_t)&kernelStack) - TMP_STACK_SIZE;
			end = (uintptr_t)&kernelStack;
		}
		return getStackTrace(bp,start,start,end);
	}
	return NULL;
}

uintptr_t *Util::getUserStackTraceOf(Thread *t) {
	uintptr_t start,end;
	if(t->getStackRange(&start,&end,0)) {
		PageDir *pdir = t->getProc()->getPageDir();

		// get base pointer
		IntrptStackFrame *istack = t->getUserState();
		frameno_t frame = pdir->getFrameNo(t->getKernelStack());
		uintptr_t temp = PageDir::getAccess(frame);
		istack = (IntrptStackFrame*)(temp + ((uintptr_t)istack & (PAGE_SIZE - 1)));
		ulong *bp = (ulong*)istack->getBP();
		PageDir::removeAccess(frame);

		// get last stack page
		frame = pdir->getFrameNo(start);
		temp = PageDir::getAccess(frame);

		// get trace
		uintptr_t *calls = getStackTrace(bp,start,temp,temp + PAGE_SIZE);

		PageDir::removeAccess(frame);
		return calls;
	}
	return NULL;
}

uintptr_t *Util::getKernelStackTraceOf(const Thread *t) {
	Thread *run = Thread::getRunning();
	if(run == t)
		return getKernelStackTrace();
	else {
		ulong bp = t->getRegs().getBP();
		frameno_t frame = t->getProc()->getPageDir()->getFrameNo(t->getKernelStack());
		uintptr_t temp = PageDir::getAccess(frame);
		uintptr_t *calls = getStackTrace((ulong*)bp,t->getKernelStack(),temp,temp + PAGE_SIZE);
		PageDir::removeAccess(frame);
		return calls;
	}
}

uintptr_t *getStackTrace(ulong *bp,uintptr_t rstart,uintptr_t mstart,uintptr_t mend) {
	static uintptr_t frames[Util::MAX_STACK_DEPTH];
	uintptr_t *frame = &frames[0];

	for(size_t i = 0; i < Util::MAX_STACK_DEPTH; i++) {
		if(bp == NULL)
			break;
		/* adjust it if we're in the kernel-stack but are using the temp-area (to print the trace
		 * for another thread). don't do this for the temp-kernel-stack */
		if(rstart != ((uintptr_t)&kernelStack) - TMP_STACK_SIZE && rstart != mstart)
			bp = (ulong*)(mstart + ((uintptr_t)bp & (PAGE_SIZE - 1)));
		/* prevent page-fault */
		if((uintptr_t)bp < mstart || (uintptr_t)(bp + 1) < mstart ||
				(((uintptr_t)(bp + 1) + sizeof(ulong) - 1) & ~(sizeof(ulong) - 1)) >= mend)
			break;
		*frame = *(bp + 1) - CALL_INSTR_SIZE;
		frame++;
		/* detect loops */
		ulong *oldbp = bp;
		bp = (ulong*)*bp;
		if(bp == oldbp)
			break;
	}

	/* terminate */
	*frame = 0;
	return &frames[0];
}
