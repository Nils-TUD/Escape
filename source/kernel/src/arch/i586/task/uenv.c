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
#include <sys/task/thread.h>
#include <sys/mem/vmm.h>
#include <sys/mem/paging.h>
#include <sys/vfs/real.h>
#include <string.h>
#include <errors.h>
#include <assert.h>

/* storage for "delayed" signal handling */
typedef struct {
	uint8_t active;
	tid_t tid;
	sig_t sig;
} sSignalData;

static void uenv_setupStack(sIntrptStackFrame *frame,uintptr_t entryPoint);
static uint32_t *uenv_addArgs(const sThread *t,uint32_t *esp,uintptr_t tentryPoint,bool newThread);

/* temporary storage for signal-handling */
static sSignalData signalData;

void uenv_handleSignal(void) {
	tid_t tid;
	sig_t sig;
	/* don't do anything, if we should already handle a signal */
	if(signalData.active == 1)
		return;

	if(sig_hasSignal(&sig,&tid)) {
		signalData.active = 1;
		signalData.sig = sig;
		signalData.tid = tid;

		/* a small trick: we store the signal to handle and manipulate the user-stack
		 * and so on later. if the thread is currently running everything is fine. we return
		 * from here and intrpt_handleSignalFinish() will be called.
		 * if the target-thread is not running we switch to him now. the thread is somewhere
		 * in the kernel but he will leave the kernel IN EVERY CASE at the end of the interrupt-
		 * handler. So if we do the signal-stuff at the end we'll get there and will
		 * manipulate the user-stack.
		 * This is simpler than mapping the user-stack and kernel-stack of the other thread
		 * in the current page-dir and so on :)
		 */
		if(thread_getRunning()->tid != tid) {
			/* ensure that the thread is ready */
			/* this may fail because perhaps we're involved in a swapping-operation or similar */
			/* in this case do nothing, we'll handle the signal later (handleSignalFinish() cares
			 * about that) */
			if(thread_setReady(tid))
				thread_switchTo(tid);
		}
	}
}

bool uenv_hasSignalToStart(void) {
	return signalData.active;
}

void uenv_startSignalHandler(sIntrptStackFrame *stack) {
	const sThread *t = thread_getRunning();
	fSignal handler;
	uint32_t *esp = (uint32_t*)stack->uesp;

	/* release signal-data */
	signalData.active = 0;

	/* if the thread_switchTo() wasn't successfull it means that we have tried to switch to
	 * multiple threads during idle. So we ignore it and try to give the signal later to the thread */
	if(t->tid != signalData.tid)
		return;

	/* extend the stack, if necessary */
	if(thread_extendStack((uintptr_t)(esp - 11)) < 0) {
		proc_segFault(t->proc);
		return;
	}
	/* will handle copy-on-write */
	paging_isRangeUserWritable((uintptr_t)(esp - 11),10 * sizeof(uint32_t));

	/* thread_extendStack() and paging_isRangeUserWritable() may cause a thread-switch. therefore
	 * we may have delivered another signal in the meanwhile... */
	if(t->tid != signalData.tid)
		return;

	handler = sig_startHandling(signalData.tid,signalData.sig);
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
	*--esp = signalData.sig;
	/* sigRet will remove the argument, restore the register,
	 * acknoledge the signal and return to eip */
	*--esp = t->proc->sigRetAddr;
	stack->eip = (uintptr_t)handler;
	stack->uesp = (uint32_t)esp;
}

int uenv_finishSignalHandler(sIntrptStackFrame *stack,sig_t signal) {
	UNUSED(signal);
	uint32_t *esp = (uint32_t*)stack->uesp;
	if(!paging_isRangeUserReadable((uintptr_t)esp,sizeof(uint32_t) * 9))
		return ERR_INVALID_ARGS;

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
	sIntrptStackFrame *frame = t->kstackEnd;

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
	vmm_getRegRange(t->proc,t->stackRegions[0],NULL,(uintptr_t*)&esp);

	/* extend the stack if necessary */
	if(thread_extendStack((uintptr_t)esp - totalSize) < 0)
		return false;
	/* will handle copy-on-write */
	paging_isRangeUserWritable((uintptr_t)esp - totalSize,totalSize);

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
	uenv_setupStack(frame,entryPoint);
	return true;
}

bool uenv_setupThread(const void *arg,uintptr_t tentryPoint) {
	uint32_t *esp;
	size_t totalSize = 3 * sizeof(uint32_t) + sizeof(void*);
	sThread *t = thread_getRunning();

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
	vmm_getRegRange(t->proc,t->stackRegions[0],NULL,(uintptr_t*)&esp);

	/* extend the stack if necessary */
	if(thread_extendStack((uintptr_t)esp - totalSize) < 0)
		return false;
	/* will handle copy-on-write */
	paging_isRangeUserWritable((uintptr_t)esp - totalSize,totalSize);

	/* put arg on stack */
	esp--;
	*esp-- = (uintptr_t)arg;
	/* add TLS args and entrypoint */
	esp = uenv_addArgs(t,esp,tentryPoint,true);

	t->kstackEnd->uesp = (uint32_t)esp;
	t->kstackEnd->ebp = t->kstackEnd->uesp;
	uenv_setupStack(t->kstackEnd,t->proc->entryPoint);
	return true;
}

static void uenv_setupStack(sIntrptStackFrame *frame,uintptr_t entryPoint) {
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
	if(t->tlsRegion >= 0) {
		uintptr_t tlsStart,tlsEnd;
		vmm_getRegRange(t->proc,t->tlsRegion,&tlsStart,&tlsEnd);
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
