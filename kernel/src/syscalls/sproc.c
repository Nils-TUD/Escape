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
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/ioports.h>
#include <sys/task/elf.h>
#include <sys/task/signals.h>
#include <sys/machine/timer.h>
#include <sys/machine/gdt.h>
#include <sys/machine/vm86.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/mem/vmm.h>
#include <sys/syscalls/proc.h>
#include <sys/syscalls.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/real.h>
#include <errors.h>
#include <string.h>

void sysc_getpid(sIntrptStackFrame *stack) {
	sProc *p = proc_getRunning();
	SYSC_RET1(stack,p->pid);
}

void sysc_getppid(sIntrptStackFrame *stack) {
	tPid pid = (tPid)SYSC_ARG1(stack);
	sProc *p;

	if(!proc_exists(pid))
		SYSC_ERROR(stack,ERR_INVALID_PID);

	p = proc_getByPid(pid);
	SYSC_RET1(stack,p->parentPid);
}

void sysc_fork(sIntrptStackFrame *stack) {
	tPid newPid = proc_getFreePid();
	s32 res;

	/* no free slot? */
	if(newPid == INVALID_PID)
		SYSC_ERROR(stack,ERR_NO_FREE_PROCS);

	res = proc_clone(newPid,false);

	/* error? */
	if(res < 0)
		SYSC_ERROR(stack,res);
	/* child? */
	if(res == 1)
		SYSC_RET1(stack,0);
	/* parent */
	else
		SYSC_RET1(stack,newPid);
}

void sysc_exit(sIntrptStackFrame *stack) {
	s32 exitCode = (s32)SYSC_ARG1(stack);
	proc_destroyThread(exitCode);
	thread_switch();
	util_panic("We shouldn't get here...");
}

void sysc_wait(sIntrptStackFrame *stack) {
	u16 events = (u8)SYSC_ARG1(stack);
	sThread *t = thread_getRunning();

	if((events & ~(EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) != 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* check whether there is a chance that we'll wake up again */
	if(!vfs_msgAvailableFor(t->tid,events)) {
		thread_wait(t->tid,0,events);
		thread_switch();
		if(sig_hasSignalFor(t->tid))
			SYSC_ERROR(stack,ERR_INTERRUPTED);
	}
	SYSC_RET1(stack,0);
}

void sysc_waitChild(sIntrptStackFrame *stack) {
	sExitState *state = (sExitState*)SYSC_ARG1(stack);
	sSLNode *n;
	s32 res;
	sProc *p = proc_getRunning();
	sThread *t = thread_getRunning();

	if(state != NULL && !paging_isRangeUserWritable((u32)state,sizeof(sExitState)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!proc_hasChild(t->proc->pid))
		SYSC_ERROR(stack,ERR_NO_CHILD);

	/* check if there is another thread waiting */
	for(n = sll_begin(p->threads); n != NULL; n = n->next) {
		if(((sThread*)n->data)->events & EV_CHILD_DIED)
			SYSC_ERROR(stack,ERR_THREAD_WAITING);
	}

	/* check if there is already a dead child-proc */
	res = proc_getExitState(p->pid,state);
	if(res < 0) {
		/* wait for child */
		thread_wait(t->tid,0,EV_CHILD_DIED);
		thread_switch();

		/* we're back again :) */
		/* stop waiting for event; maybe we have been waked up for another reason */
		thread_wakeup(t->tid,EV_CHILD_DIED);
		/* don't continue here if we were interrupted by a signal */
		if(sig_hasSignalFor(t->tid))
			SYSC_ERROR(stack,ERR_INTERRUPTED);
		res = proc_getExitState(p->pid,state);
		if(res < 0)
			SYSC_ERROR(stack,res);
	}

	/* finally kill the process */
	proc_kill(proc_getByPid(res));
	SYSC_RET1(stack,0);
}

void sysc_sleep(sIntrptStackFrame *stack) {
	u32 msecs = SYSC_ARG1(stack);
	sThread *t = thread_getRunning();
	timer_sleepFor(t->tid,msecs);
	thread_switch();
	if(sig_hasSignalFor(t->tid))
		SYSC_ERROR(stack,ERR_INTERRUPTED);
	SYSC_RET1(stack,0);
}

void sysc_yield(sIntrptStackFrame *stack) {
	UNUSED(stack);
	thread_switch();
}

void sysc_requestIOPorts(sIntrptStackFrame *stack) {
	u16 start = SYSC_ARG1(stack);
	u16 count = SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	s32 err;

	/* check range */
	if(count == 0 || (u32)start + (u32)count > 0xFFFF)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	err = ioports_request(p,start,count);
	if(err < 0)
		SYSC_ERROR(stack,err);

	tss_setIOMap(p->ioMap);
	SYSC_RET1(stack,0);
}

void sysc_releaseIOPorts(sIntrptStackFrame *stack) {
	u16 start = SYSC_ARG1(stack);
	u16 count = SYSC_ARG2(stack);
	sProc *p;
	s32 err;

	/* check range */
	if(count == 0 || (u32)start + (u32)count > 0xFFFF)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	p = proc_getRunning();
	err = ioports_release(p,start,count);
	if(err < 0)
		SYSC_ERROR(stack,err);

	tss_setIOMap(p->ioMap);
	SYSC_RET1(stack,0);
}

void sysc_exec(sIntrptStackFrame *stack) {
	char pathSave[MAX_PATH_LEN + 1];
	char *path = (char*)SYSC_ARG1(stack);
	char **args = (char**)SYSC_ARG2(stack);
	char *argBuffer;
	sStartupInfo info;
	s32 argc,pathLen,res;
	u32 argSize;
	tInodeNo nodeNo;
	sThread *t = thread_getRunning();
	sProc *p = t->proc;

	argc = 0;
	argBuffer = NULL;
	if(args != NULL) {
		argc = proc_buildArgs(args,&argBuffer,&argSize,true);
		if(argc < 0)
			SYSC_ERROR(stack,argc);
	}

	/* at first make sure that we'll cause no page-fault */
	if(!sysc_isStringReadable(path)) {
		kheap_free(argBuffer);
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}

	/* save path (we'll overwrite the process-data) */
	pathLen = strlen(path);
	if(pathLen >= MAX_PATH_LEN) {
		kheap_free(argBuffer);
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}
	memcpy(pathSave,path,pathLen + 1);
	path = pathSave;

	/* resolve path; require a path in real fs */
	res = vfsn_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res != ERR_REAL_PATH) {
		kheap_free(argBuffer);
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}

	/* remove all except stack */
	proc_removeRegions(p,false);

	/* load program */
	if(elf_loadFromFile(path,&info) < 0)
		goto error;

	/* copy path so that we can identify the process */
	memcpy(p->command,pathSave + (pathLen > MAX_PROC_NAME_LEN ? (pathLen - MAX_PROC_NAME_LEN) : 0),
			MIN(MAX_PROC_NAME_LEN,pathLen) + 1);

	/* make process ready */
	if(!proc_setupUserStack(stack,argc,argBuffer,argSize,&info))
		goto error;
	/* for starting use the linker-entry, which will be progEntry if no dl is present */
	proc_setupStart(stack,info.linkerEntry);

	/* if its the dynamic linker, open the program to exec and give him the filedescriptor,
	 * so that he can load it including all shared libraries */
	if(info.linkerEntry != info.progEntry) {
		u32 *esp = (u32*)stack->uesp;
		tFileNo file;
		tFD fd = thread_getFreeFd();
		if(fd < 0)
			goto error;
		file = vfsr_openFile(t->tid,VFS_READ,path);
		if(file < 0)
			goto error;
		assert(thread_assocFd(fd,file) == 0);
		*--esp = fd;
		stack->uesp = (u32)esp;
	}

	kheap_free(argBuffer);
	return;

error:
	kheap_free(argBuffer);
	proc_terminate(p,res,SIG_COUNT);
	thread_switch();
}

void sysc_vm86int(sIntrptStackFrame *stack) {
	s32 res;
	u16 interrupt = (u16)SYSC_ARG1(stack);
	sVM86Regs *regs = (sVM86Regs*)SYSC_ARG2(stack);
	sVM86Memarea *mAreas = (sVM86Memarea*)SYSC_ARG3(stack);
	u16 mAreaCount = (u16)SYSC_ARG4(stack);

	/* check args */
	if(regs == NULL)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isRangeUserWritable((u32)regs,sizeof(sVM86Regs)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(mAreas != NULL) {
		u32 i;
		if(!paging_isRangeUserReadable((u32)mAreas,sizeof(sVM86Memarea) * mAreaCount))
			SYSC_ERROR(stack,ERR_INVALID_ARGS);
		for(i = 0; i < mAreaCount; i++) {
			/* ensure that just something from the real-mode-memory can be copied */
			if(mAreas[i].type == VM86_MEM_DIRECT) {
				if(mAreas[i].data.direct.dst + mAreas[i].data.direct.size < mAreas[i].data.direct.dst ||
					mAreas[i].data.direct.dst + mAreas[i].data.direct.size >= (1 * M + 64 * K))
					SYSC_ERROR(stack,ERR_INVALID_ARGS);
				if(!paging_isRangeUserWritable((u32)mAreas[i].data.direct.src,mAreas[i].data.direct.size))
					SYSC_ERROR(stack,ERR_INVALID_ARGS);
			}
			else {
				if(!paging_isRangeUserReadable((u32)mAreas[i].data.ptr.srcPtr,sizeof(u16*)))
					SYSC_ERROR(stack,ERR_INVALID_ARGS);
				if(!paging_isRangeUserWritable(mAreas[i].data.ptr.result,mAreas[i].data.ptr.size))
					SYSC_ERROR(stack,ERR_INVALID_ARGS);
			}
		}
	}
	else
		mAreaCount = 0;

	/* do vm86-interrupt */
	res = vm86_int(interrupt,regs,mAreas,mAreaCount);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_getCycles(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	uLongLong cycles;
	cycles.val64 = t->stats.kcycleCount.val64 + t->stats.ucycleCount.val64;
	SYSC_RET1(stack,cycles.val32.lower);
	SYSC_RET2(stack,cycles.val32.upper);
}
