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
#include <task/proc.h>
#include <task/thread.h>
#include <task/ioports.h>
#include <task/elf.h>
#include <task/signals.h>
#include <machine/timer.h>
#include <machine/gdt.h>
#include <mem/paging.h>
#include <mem/kheap.h>
#include <syscalls/proc.h>
#include <syscalls.h>
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

	res = proc_clone(newPid);

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
}

void sysc_wait(sIntrptStackFrame *stack) {
	u8 events = (u8)SYSC_ARG1(stack);
	sThread *t = thread_getRunning();

	if((events & ~(EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) != 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* check wether there is a chance that we'll wake up again */
	if(!vfs_msgAvailableFor(t->tid,events)) {
		thread_wait(t->tid,events);
		thread_switch();
	}
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
		thread_wait(t->tid,EV_CHILD_DIED);
		thread_switch();

		/* we're back again :) */
		/* don't continue here if we were interrupted by a signal */
		if(sig_hasSignalFor(t->tid)) {
			/* stop waiting for event */
			thread_wakeup(t->tid,EV_CHILD_DIED);
			SYSC_ERROR(stack,ERR_INTERRUPTED);
		}
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
	timer_sleepFor(thread_getRunning()->tid,msecs);
	thread_switch();
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
	s32 argc,pathLen,res;
	u32 argSize;
	tInodeNo nodeNo;
	sProc *p = proc_getRunning();

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

	/* free the current text; free frames if text_free() returns true */
	paging_unmap(0,p->textPages,text_free(p->text,p->pid),false);
	/* ensure that we don't have a text-usage anymore */
	p->text = NULL;
	/* remove process-data */
	proc_changeSize(-p->dataPages,CHG_DATA);
	/* Note that we HAVE TO do it behind the proc_changeSize() call since the data-pages are
	 * still behind the text-pages, no matter if we've already unmapped the text-pages or not,
	 * and proc_changeSize() trusts p->textPages */
	p->textPages = 0;

	/* load program */
	res = elf_loadFromFile(path);
	if(res < 0) {
		/* there is no undo for proc_changeSize() :/ */
		kheap_free(argBuffer);
		proc_terminate(p,res,SIG_COUNT);
		thread_switch();
		return;
	}

	/* copy path so that we can identify the process */
	memcpy(p->command,pathSave + (pathLen > MAX_PROC_NAME_LEN ? (pathLen - MAX_PROC_NAME_LEN) : 0),
			MIN(MAX_PROC_NAME_LEN,pathLen) + 1);

	/* make process ready */
	proc_setupUserStack(stack,argc,argBuffer,argSize);
	proc_setupStart(stack,(u32)res);

	kheap_free(argBuffer);
}

void sysc_getCycles(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	uLongLong cycles;
	cycles.val64 = t->kcycleCount.val64 + t->ucycleCount.val64;
	SYSC_RET1(stack,cycles.val32.upper);
	SYSC_RET2(stack,cycles.val32.lower);
}
