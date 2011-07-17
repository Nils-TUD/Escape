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
#include <sys/vfs/real.h>
#include <sys/util.h>
#include <sys/debug.h>
#include <errors.h>
#include <string.h>

static ssize_t sysc_copyEnv(const char *src,char *dst,size_t size);

int sysc_getpid(sIntrptStackFrame *stack) {
	const sProc *p = proc_getRunning();
	SYSC_RET1(stack,p->pid);
}

int sysc_getppid(sIntrptStackFrame *stack) {
	pid_t pid = (pid_t)SYSC_ARG1(stack);
	const sProc *p;

	if(!proc_exists(pid))
		SYSC_ERROR(stack,ERR_INVALID_PID);

	p = proc_getByPid(pid);
	SYSC_RET1(stack,p->parentPid);
}

int sysc_getuid(sIntrptStackFrame *stack) {
	const sProc *p = proc_getRunning();
	SYSC_RET1(stack,p->ruid);
}

int sysc_setuid(sIntrptStackFrame *stack) {
	uid_t uid = (uid_t)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	if(p->euid != ROOT_UID)
		SYSC_ERROR(stack,ERR_NO_PERM);

	p->ruid = uid;
	p->euid = uid;
	p->suid = uid;
	SYSC_RET1(stack,0);
}

int sysc_getgid(sIntrptStackFrame *stack) {
	const sProc *p = proc_getRunning();
	SYSC_RET1(stack,p->rgid);
}

int sysc_setgid(sIntrptStackFrame *stack) {
	gid_t gid = (gid_t)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	if(p->euid != ROOT_UID)
		SYSC_ERROR(stack,ERR_NO_PERM);

	p->rgid = gid;
	p->egid = gid;
	p->sgid = gid;
	SYSC_RET1(stack,0);
}

int sysc_geteuid(sIntrptStackFrame *stack) {
	const sProc *p = proc_getRunning();
	SYSC_RET1(stack,p->euid);
}

int sysc_seteuid(sIntrptStackFrame *stack) {
	uid_t uid = (uid_t)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	/* if not root, it has to be either ruid, euid or suid */
	if(p->euid != ROOT_UID && uid != p->ruid && uid != p->euid && uid != p->suid)
		SYSC_ERROR(stack,ERR_NO_PERM);

	p->euid = uid;
	SYSC_RET1(stack,0);
}

int sysc_getegid(sIntrptStackFrame *stack) {
	const sProc *p = proc_getRunning();
	SYSC_RET1(stack,p->egid);
}

int sysc_setegid(sIntrptStackFrame *stack) {
	gid_t gid = (gid_t)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	/* if not root, it has to be either rgid, egid or sgid */
	if(p->euid != ROOT_UID && gid != p->rgid && gid != p->egid && gid != p->sgid)
		SYSC_ERROR(stack,ERR_NO_PERM);

	p->egid = gid;
	SYSC_RET1(stack,0);
}

int sysc_getgroups(sIntrptStackFrame *stack) {
	size_t size = (size_t)SYSC_ARG1(stack);
	gid_t *list = (gid_t*)SYSC_ARG2(stack);
	const sProc *p = proc_getRunning();
	const sProcGroups *g = p->groups;
	size_t groupCount = g ? g->count : 0;
	if(size == 0)
		SYSC_RET1(stack,groupCount);

	if(!paging_isRangeUserWritable((uintptr_t)list,sizeof(gid_t) * size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	size = MIN(groupCount,size);
	if(size > 0)
		memcpy(list,g->groups,size * sizeof(gid_t));
	SYSC_RET1(stack,size);
}

int sysc_setgroups(sIntrptStackFrame *stack) {
	size_t size = (size_t)SYSC_ARG1(stack);
	const gid_t *list = (const gid_t*)SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	sProcGroups *g;

	if(size > 0 && !paging_isRangeUserReadable((uintptr_t)list,sizeof(gid_t) * size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	g = groups_alloc(size,list);
	if(!g)
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
	groups_leave(p->groups);
	p->groups = g;
	SYSC_RET1(stack,0);
}

int sysc_isingroup(sIntrptStackFrame *stack) {
	pid_t pid = (pid_t)SYSC_ARG1(stack);
	gid_t gid = (gid_t)SYSC_ARG2(stack);
	const sProc *p = proc_getByPid(pid);
	if(!p)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	SYSC_RET1(stack,groups_contains(p->groups,gid));
}

int sysc_fork(sIntrptStackFrame *stack) {
	pid_t newPid = proc_getFreePid();
	int res;

	/* no free slot? */
	if(newPid == INVALID_PID)
		SYSC_ERROR(stack,ERR_NO_FREE_PROCS);

	res = proc_clone(newPid,0);

	/* error? */
	if(res < 0)
		SYSC_ERROR(stack,res);
	/* child? */
	if(res == 1)
		SYSC_RET1(stack,0);
	/* parent */
	SYSC_RET1(stack,newPid);
}

int sysc_waitChild(sIntrptStackFrame *stack) {
	sExitState *state = (sExitState*)SYSC_ARG1(stack);
	sSLNode *n;
	int res;
	const sThread *t = thread_getRunning();
	const sProc *p = t->proc;

	if(state != NULL && !paging_isRangeUserWritable((uintptr_t)state,sizeof(sExitState)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* check if there is another thread waiting */
	for(n = sll_begin(p->threads); n != NULL; n = n->next) {
		sThread *ot = (sThread*)n->data;
		if(ev_waitsFor(ot->tid,EV_CHILD_DIED))
			SYSC_ERROR(stack,ERR_THREAD_WAITING);
	}

	/* check if there is already a dead child-proc */
	res = proc_getExitState(p->pid,state);
	if(res < 0) {
		/* wait for child */
		ev_wait(t->tid,EVI_CHILD_DIED,(evobj_t)p);
		thread_switch();
		/* stop waiting for event; maybe we have been waked up for another reason */
		ev_removeThread(t->tid);
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

int sysc_getenvito(sIntrptStackFrame *stack) {
	char *buffer = (char*)SYSC_ARG1(stack);
	size_t size = SYSC_ARG2(stack);
	size_t index = SYSC_ARG3(stack);
	const sProc *p = proc_getRunning();
	ssize_t res;

	const char *name = env_geti(p->pid,index);
	if(name == NULL)
		SYSC_ERROR(stack,ERR_ENVVAR_NOT_FOUND);

	res = sysc_copyEnv(name,buffer,size);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_getenvto(sIntrptStackFrame *stack) {
	char *buffer = (char*)SYSC_ARG1(stack);
	size_t size = SYSC_ARG2(stack);
	const char *name = (const char*)SYSC_ARG3(stack);
	const sProc *p = proc_getRunning();
	ssize_t res;

	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	const char *value = env_get(p->pid,name);
	if(value == NULL)
		SYSC_ERROR(stack,ERR_ENVVAR_NOT_FOUND);

	res = sysc_copyEnv(value,buffer,size);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_setenv(sIntrptStackFrame *stack) {
	const char *name = (const char*)SYSC_ARG1(stack);
	const char *value = (const char*)SYSC_ARG2(stack);
	const sProc *p = proc_getRunning();

	if(!sysc_isStringReadable(name) || !sysc_isStringReadable(value))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	if(!env_set(p->pid,name,value))
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);
	SYSC_RET1(stack,0);
}

int sysc_exec(sIntrptStackFrame *stack) {
	char pathSave[MAX_PATH_LEN + 1];
	char *path = (char*)SYSC_ARG1(stack);
	const char *const *args = (const char *const *)SYSC_ARG2(stack);
	char *argBuffer;
	sStartupInfo info;
	inode_t nodeNo;
	sProc *p = proc_getRunning();
	ssize_t pathLen;
	size_t argSize;
	int argc,res;

	argc = 0;
	argBuffer = NULL;
	if(args != NULL) {
		argc = proc_buildArgs(args,&argBuffer,&argSize,true);
		if(argc < 0)
			SYSC_ERROR(stack,argc);
	}

	/* at first make sure that we'll cause no page-fault */
	if(!sysc_isStringReadable(path)) {
		cache_free(argBuffer);
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}

	/* save path (we'll overwrite the process-data) */
	pathLen = strlen(path);
	if(pathLen >= MAX_PATH_LEN) {
		cache_free(argBuffer);
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}
	if(*path != '/')
		path = vfs_node_absolutize(pathSave,sizeof(pathSave),path);
	else
		path = memcpy(pathSave,path,pathLen + 1);

	/* resolve path; require a path in real fs */
	res = vfs_node_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res != ERR_REAL_PATH) {
		cache_free(argBuffer);
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}

	/* remove all except stack */
	proc_removeRegions(p,false);

	/* load program */
	if(elf_loadFromFile(path,&info) < 0)
		goto error;

	/* copy path so that we can identify the process */
	proc_setCommand(p,pathSave);

#ifdef __eco32__
	debugf("EXEC: proc %d:%s\n",p->pid,p->command);
#endif
#ifdef __mmix__
	debugf("EXEC: proc %d:%s\n",p->pid,p->command);
#endif

	/* make process ready */
	/* the entry-point is the one of the process, since threads don't start with the dl again */
	p->entryPoint = info.progEntry;
	/* for starting use the linker-entry, which will be progEntry if no dl is present */
	if(!uenv_setupProc(path,argc,argBuffer,argSize,&info,info.linkerEntry))
		goto error;

	cache_free(argBuffer);
	SYSC_RET1(stack,0);

error:
	cache_free(argBuffer);
	proc_terminate(p,res,SIG_COUNT);
	thread_switch();
	util_panic("We should not reach this!");
	SYSC_ERROR(stack,res);
}

static ssize_t sysc_copyEnv(const char *src,char *dst,size_t size) {
	size_t len;
	if(size == 0 || !paging_isRangeUserWritable((uintptr_t)dst,size))
		return ERR_INVALID_ARGS;

	/* copy to buffer */
	len = strlen(src);
	len = MIN(len,size - 1);
	strncpy(dst,src,len);
	dst[len] = '\0';
	return len;
}
