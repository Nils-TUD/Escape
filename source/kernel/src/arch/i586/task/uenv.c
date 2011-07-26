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
#include <sys/arch/i586/gdt.h>
#include <sys/task/uenv.h>
#include <sys/task/event.h>
#include <sys/task/thread.h>
#include <sys/mem/vmm.h>
#include <sys/mem/paging.h>
#include <sys/vfs/real.h>
#include <string.h>
#include <errors.h>
#include <assert.h>

static void uenv_startSignalHandler(sThread *t,sIntrptStackFrame *stack,sig_t sig);
static void uenv_setupRegs(sIntrptStackFrame *frame,uintptr_t entryPoint);
static uint32_t *uenv_addArgs(const sThread *t,uint32_t *esp,uintptr_t tentryPoint,bool newThread);

void uenv_handleSignal(sIntrptStackFrame *stack) {
	tid_t tid;
	sig_t sig;
	sThread *t = thread_getRunning();
	if((sig = thread_getSignal(t)) != SIG_COUNT)
		uenv_startSignalHandler(t,stack,sig);

	if(sig_hasSignal(&sig,&tid)) {
		if(t->tid == tid)
			uenv_startSignalHandler(t,stack,sig);
		else {
			t = thread_getById(tid);
			if(thread_setSignal(t,sig)) {
				ev_unblock(t);
				thread_switchTo(tid);
			}
		}
	}
}

int uenv_finishSignalHandler(sIntrptStackFrame *stack,sig_t signal) {
	UNUSED(signal);
	uint32_t *esp = (uint32_t*)stack->uesp;
	/* remove arg */
	esp += 1;
	/* restore regs */
	stack->esi = *esp++;
	stack->edi = *esp++;
	stack->edx = *esp++;
	stack->ecx = *esp++;
	stack->ebx = *esp++;
	stack->eax = *esp++;
	stack->eflags = *esp++;
	/* return */
	stack->eip = *esp++;
	stack->uesp = (uintptr_t)esp;
	return 0;
}

bool uenv_setupProc(const char *path,int argc,const char *args,size_t argsSize,
		const sStartupInfo *info,uintptr_t entryPoint) {
	uint32_t *esp;
	char **argv;
	size_t totalSize;
	const sThread *t = thread_getRunning();
	sIntrptStackFrame *frame = thread_getIntrptStack(t);

	/*
	 * Initial stack:
	 * +------------------+  <- top
	 * |     arguments    |
	 * |        ...       |
	 * +------------------+
	 * |       argv       |
	 * +------------------+
	 * |       argc       |
	 * +------------------+
	 * |     TLSSize      |  0 if not present
	 * +------------------+
	 * |     TLSStart     |  0 if not present
	 * +------------------+
	 * |    entryPoint    |  0 for initial thread, thread-entrypoint for others
	 * +------------------+
	 */

	/* we need to know the total number of bytes we'll store on the stack */
	totalSize = 0;
	if(argc > 0) {
		/* first round the size of the arguments up. then we need argc+1 pointer */
		totalSize += (argsSize + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1);
		totalSize += sizeof(void*) * (argc + 1);
	}
	/* finally we need argc, argv, tlsSize, tlsStart and entryPoint */
	totalSize += sizeof(uint32_t) * 5;

	/* get esp */
	thread_getStackRange(t,NULL,(uintptr_t*)&esp,0);

	/* copy arguments on the user-stack (4byte space) */
	esp--;
	argv = NULL;
	if(argc > 0) {
		char *str;
		int i;
		size_t len;
		argv = (char**)(esp - argc);
		/* space for the argument-pointer */
		esp -= argc;
		/* start for the arguments */
		str = (char*)esp;
		for(i = 0; i < argc; i++) {
			/* start <len> bytes backwards */
			len = strlen(args) + 1;
			str -= len;
			/* store arg-pointer and copy arg */
			argv[i] = str;
			memcpy(str,args,len);
			/* to next */
			args += len;
		}
		/* ensure that we don't overwrites the characters */
		esp = (uint32_t*)(((uintptr_t)str & ~(sizeof(uint32_t) - 1)) - sizeof(uint32_t));
	}

	/* store argc and argv */
	*esp-- = (uintptr_t)argv;
	*esp-- = argc;
	/* add TLS args and entrypoint; use prog-entry here because its always the entry of the
	 * program, not the dynamic-linker */
	esp = uenv_addArgs(t,esp,info->progEntry,false);

	/* if its the dynamic linker, open the program to exec and give him the filedescriptor,
	 * so that he can load it including all shared libraries */
	if(info->linkerEntry != info->progEntry) {
		file_t file;
		int fd = proc_getFreeFd();
		if(fd < 0)
			return false;
		file = vfs_real_openPath(t->proc->pid,VFS_READ,path);
		if(file < 0)
			return false;
		assert(proc_assocFd(fd,file) == 0);
		*--esp = fd;
	}

	frame->uesp = (uint32_t)esp;
	frame->ebp = frame->uesp;
	uenv_setupRegs(frame,entryPoint);
	return true;
}

bool uenv_setupThread(const void *arg,uintptr_t tentryPoint) {
	uint32_t *esp;
	sThread *t = thread_getRunning();
	sIntrptStackFrame *kstack = thread_getIntrptStack(t);

	/*
	 * Initial stack:
	 * +------------------+  <- top
	 * |       arg        |
	 * +------------------+
	 * |     TLSSize      |  0 if not present
	 * +------------------+
	 * |     TLSStart     |  0 if not present
	 * +------------------+
	 * |    entryPoint    |  0 for initial thread, thread-entrypoint for others
	 * +------------------+
	 */

	/* get esp */
	thread_getStackRange(t,NULL,(uintptr_t*)&esp,0);

	/* put arg on stack */
	esp--;
	*esp-- = (uintptr_t)arg;
	/* add TLS args and entrypoint */
	esp = uenv_addArgs(t,esp,tentryPoint,true);

	kstack->uesp = (uint32_t)esp;
	kstack->ebp = kstack->uesp;
	uenv_setupRegs(kstack,t->proc->entryPoint);
	return true;
}

static void uenv_startSignalHandler(sThread *t,sIntrptStackFrame *stack,sig_t sig) {
	fSignal handler;
	uint32_t *esp = (uint32_t*)stack->uesp;

	thread_unsetSignal(t);
	handler = sig_startHandling(t->tid,sig);
	/* the ret-instruction of sigRet() should go to the old eip */
	*--esp = stack->eip;
	/* save regs */
	*--esp = stack->eflags;
	*--esp = stack->eax;
	*--esp = stack->ebx;
	*--esp = stack->ecx;
	*--esp = stack->edx;
	*--esp = stack->edi;
	*--esp = stack->esi;
	/* signal-number as arguments */
	*--esp = sig;
	/* sigRet will remove the argument, restore the register,
	 * acknoledge the signal and return to eip */
	*--esp = t->proc->sigRetAddr;
	stack->eip = (uintptr_t)handler;
	stack->uesp = (uint32_t)esp;
}

static void uenv_setupRegs(sIntrptStackFrame *frame,uintptr_t entryPoint) {
	/* user-mode segments */
	frame->cs = SEGSEL_GDTI_UCODE | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->ds = SEGSEL_GDTI_UDATA | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->es = SEGSEL_GDTI_UDATA | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->fs = SEGSEL_GDTI_UDATA | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	/* gs is used for TLS */
	frame->gs = SEGSEL_GDTI_UTLS | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->uss = SEGSEL_GDTI_UDATA | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->eip = entryPoint;
	/* general purpose register */
	frame->eax = 0;
	frame->ebx = 0;
	frame->ecx = 0;
	frame->edx = 0;
	frame->esi = 0;
	frame->edi = 0;
}

static uint32_t *uenv_addArgs(const sThread *t,uint32_t *esp,uintptr_t tentryPoint,bool newThread) {
	/* put address and size of the tls-region on the stack */
	uintptr_t tlsStart,tlsEnd;
	if(thread_getTLSRange(t,&tlsStart,&tlsEnd)) {
		*esp-- = tlsEnd - tlsStart;
		*esp-- = tlsStart;
	}
	else {
		/* no tls */
		*esp-- = 0;
		*esp-- = 0;
	}

	*esp = newThread ? tentryPoint : 0;
	return esp;
}
