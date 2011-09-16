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
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/event.h>
#include <sys/task/elf.h>
#include <sys/task/signals.h>
#include <sys/task/env.h>
#include <sys/task/uenv.h>
#include <sys/task/timer.h>
#include <sys/task/groups.h>
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/mem/vmm.h>
#include <sys/syscalls/proc.h>
#include <sys/syscalls.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/real.h>
#include <sys/util.h>
#include <sys/debug.h>
#include <errors.h>
#include <string.h>

int sysc_getpid(sThread *t,sIntrptStackFrame *stack) {
	SYSC_RET1(stack,t->proc->pid);
}

int sysc_getppid(A_UNUSED sThread *t,sIntrptStackFrame *stack) {
	pid_t pid = (pid_t)SYSC_ARG1(stack);
	sProc *p = proc_getByPid(pid);
	if(!p)
		SYSC_ERROR(stack,ERR_INVALID_PID);

	SYSC_RET1(stack,p->parentPid);
}

int sysc_getuid(sThread *t,sIntrptStackFrame *stack) {
	SYSC_RET1(stack,t->proc->ruid);
}

int sysc_setuid(sThread *t,sIntrptStackFrame *stack) {
	uid_t uid = (uid_t)SYSC_ARG1(stack);
	sProc *p = t->proc;
	if(p->euid != ROOT_UID)
		SYSC_ERROR(stack,ERR_NO_PERM);

	p->ruid = uid;
	p->euid = uid;
	p->suid = uid;
	SYSC_RET1(stack,0);
}

int sysc_getgid(sThread *t,sIntrptStackFrame *stack) {
	SYSC_RET1(stack,t->proc->rgid);
}

int sysc_setgid(sThread *t,sIntrptStackFrame *stack) {
	gid_t gid = (gid_t)SYSC_ARG1(stack);
	sProc *p = t->proc;
	if(p->euid != ROOT_UID)
		SYSC_ERROR(stack,ERR_NO_PERM);

	p->rgid = gid;
	p->egid = gid;
	p->sgid = gid;
	SYSC_RET1(stack,0);
}

int sysc_geteuid(sThread *t,sIntrptStackFrame *stack) {
	SYSC_RET1(stack,t->proc->euid);
}

int sysc_seteuid(sThread *t,sIntrptStackFrame *stack) {
	uid_t uid = (uid_t)SYSC_ARG1(stack);
	sProc *p = t->proc;
	/* if not root, it has to be either ruid, euid or suid */
	if(p->euid != ROOT_UID && uid != p->ruid && uid != p->euid && uid != p->suid)
		SYSC_ERROR(stack,ERR_NO_PERM);

	p->euid = uid;
	SYSC_RET1(stack,0);
}

int sysc_getegid(sThread *t,sIntrptStackFrame *stack) {
	SYSC_RET1(stack,t->proc->egid);
}

int sysc_setegid(sThread *t,sIntrptStackFrame *stack) {
	gid_t gid = (gid_t)SYSC_ARG1(stack);
	sProc *p = t->proc;
	/* if not root, it has to be either rgid, egid or sgid */
	if(p->euid != ROOT_UID && gid != p->rgid && gid != p->egid && gid != p->sgid)
		SYSC_ERROR(stack,ERR_NO_PERM);

	p->egid = gid;
	SYSC_RET1(stack,0);
}

int sysc_getgroups(sThread *t,sIntrptStackFrame *stack) {
	size_t size = (size_t)SYSC_ARG1(stack);
	gid_t *list = (gid_t*)SYSC_ARG2(stack);
	pid_t pid = t->proc->pid;
	if(!paging_isInUserSpace((uintptr_t)list,sizeof(gid_t) * size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	size = groups_get(pid,list,size);
	SYSC_RET1(stack,size);
}

int sysc_setgroups(sThread *t,sIntrptStackFrame *stack) {
	size_t size = (size_t)SYSC_ARG1(stack);
	const gid_t *list = (const gid_t*)SYSC_ARG2(stack);
	pid_t pid = t->proc->pid;
	if(!paging_isInUserSpace((uintptr_t)list,sizeof(gid_t) * size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	if(!groups_set(pid,size,list))
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
	SYSC_RET1(stack,0);
}

int sysc_isingroup(A_UNUSED sThread *t,sIntrptStackFrame *stack) {
	pid_t pid = (pid_t)SYSC_ARG1(stack);
	gid_t gid = (gid_t)SYSC_ARG2(stack);
	SYSC_RET1(stack,groups_contains(pid,gid));
}

int sysc_fork(A_UNUSED sThread *t,sIntrptStackFrame *stack) {
	int res = proc_clone(0);
	/* error? */
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_waitChild(sThread *t,sIntrptStackFrame *stack) {
	sExitState *state = (sExitState*)SYSC_ARG1(stack);
	int res;
	sProc *p = t->proc;

	if(state != NULL && !paging_isInUserSpace((uintptr_t)state,sizeof(sExitState)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* check if there is already a dead child-proc */
	res = proc_getExitState(p->pid,state);
	if(res < 0) {
		/* wait for child */
		ev_wait(t,EVI_CHILD_DIED,(evobj_t)p);
		thread_switch();
		/* stop waiting for event; maybe we have been waked up for another reason */
		ev_removeThread(t);
		/* don't continue here if we were interrupted by a signal */
		if(sig_hasSignalFor(t->tid))
			SYSC_ERROR(stack,ERR_INTERRUPTED);
		res = proc_getExitState(p->pid,state);
		if(res < 0)
			SYSC_ERROR(stack,res);
	}

	/* finally kill the process */
	proc_kill(res);
	SYSC_RET1(stack,0);
}

int sysc_getenvito(sThread *t,sIntrptStackFrame *stack) {
	char *buffer = (char*)SYSC_ARG1(stack);
	size_t size = SYSC_ARG2(stack);
	size_t index = SYSC_ARG3(stack);
	pid_t pid = t->proc->pid;
	if(size == 0 || !paging_isInUserSpace((uintptr_t)buffer,size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	if(!env_geti(pid,index,buffer,size))
		SYSC_ERROR(stack,ERR_ENVVAR_NOT_FOUND);
	SYSC_RET1(stack,0);
}

int sysc_getenvto(sThread *t,sIntrptStackFrame *stack) {
	char *buffer = (char*)SYSC_ARG1(stack);
	size_t size = SYSC_ARG2(stack);
	const char *name = (const char*)SYSC_ARG3(stack);
	pid_t pid = t->proc->pid;
	if(!sysc_isStrInUserSpace(name,NULL))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(size == 0 || !paging_isInUserSpace((uintptr_t)buffer,size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	if(!env_get(pid,name,buffer,size))
		SYSC_ERROR(stack,ERR_ENVVAR_NOT_FOUND);
	SYSC_RET1(stack,0);
}

int sysc_setenv(sThread *t,sIntrptStackFrame *stack) {
	const char *name = (const char*)SYSC_ARG1(stack);
	const char *value = (const char*)SYSC_ARG2(stack);
	pid_t pid = t->proc->pid;
	if(!sysc_isStrInUserSpace(name,NULL) || !sysc_isStrInUserSpace(value,NULL))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	if(!env_set(pid,name,value))
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
	SYSC_RET1(stack,0);
}

int sysc_exec(A_UNUSED sThread *t,sIntrptStackFrame *stack) {
	char pathSave[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	const char *const *args = (const char *const *)SYSC_ARG2(stack);
	int res;
	if(!sysc_absolutize_path(pathSave,sizeof(pathSave),path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = proc_exec(pathSave,args,NULL,0);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
