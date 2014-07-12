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

#include <common.h>
#include <task/proc.h>
#include <task/thread.h>
#include <task/elf.h>
#include <task/signals.h>
#include <task/uenv.h>
#include <task/timer.h>
#include <task/groups.h>
#include <mem/pagedir.h>
#include <mem/cache.h>
#include <mem/virtmem.h>
#include <mem/useraccess.h>
#include <syscalls.h>
#include <vfs/vfs.h>
#include <vfs/node.h>
#include <util.h>
#include <errno.h>
#include <string.h>

int Syscalls::getpid(Thread *t,IntrptStackFrame *stack) {
	SYSC_RET1(stack,t->getProc()->getPid());
}

int Syscalls::getppid(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	pid_t pid = (pid_t)SYSC_ARG1(stack);
	Proc *p = Proc::getRef(pid);
	if(EXPECT_FALSE(!p))
		SYSC_ERROR(stack,-ESRCH);

	pid_t ppid = p->getParentPid();
	Proc::relRef(p);
	SYSC_RET1(stack,ppid);
}

int Syscalls::getuid(Thread *t,IntrptStackFrame *stack) {
	SYSC_RET1(stack,t->getProc()->getRUid());
}

int Syscalls::setuid(Thread *t,IntrptStackFrame *stack) {
	uid_t uid = (uid_t)SYSC_ARG1(stack);
	Proc *p = t->getProc();
	if(EXPECT_FALSE(p->getEUid() != ROOT_UID))
		SYSC_ERROR(stack,-EPERM);

	p->setUid(uid);
	SYSC_RET1(stack,0);
}

int Syscalls::getgid(Thread *t,IntrptStackFrame *stack) {
	SYSC_RET1(stack,t->getProc()->getRGid());
}

int Syscalls::setgid(Thread *t,IntrptStackFrame *stack) {
	gid_t gid = (gid_t)SYSC_ARG1(stack);
	Proc *p = t->getProc();
	if(EXPECT_FALSE(p->getEUid() != ROOT_UID))
		SYSC_ERROR(stack,-EPERM);

	p->setGid(gid);
	SYSC_RET1(stack,0);
}

int Syscalls::geteuid(Thread *t,IntrptStackFrame *stack) {
	SYSC_RET1(stack,t->getProc()->getEUid());
}

int Syscalls::seteuid(Thread *t,IntrptStackFrame *stack) {
	uid_t uid = (uid_t)SYSC_ARG1(stack);
	Proc *p = t->getProc();
	/* if not root, it has to be either ruid, euid or suid */
	if(EXPECT_FALSE(p->getEUid() != ROOT_UID && uid != p->getRUid() && uid != p->getEUid() &&
			uid != p->getSUid()))
		SYSC_ERROR(stack,-EPERM);

	p->setEUid(uid);
	SYSC_RET1(stack,0);
}

int Syscalls::getegid(Thread *t,IntrptStackFrame *stack) {
	SYSC_RET1(stack,t->getProc()->getEGid());
}

int Syscalls::setegid(Thread *t,IntrptStackFrame *stack) {
	gid_t gid = (gid_t)SYSC_ARG1(stack);
	Proc *p = t->getProc();
	/* if not root, it has to be either rgid, egid or sgid */
	if(EXPECT_FALSE(p->getEUid() != ROOT_UID && gid != p->getRGid() && gid != p->getEGid() &&
			gid != p->getSGid()))
		SYSC_ERROR(stack,-EPERM);

	p->setEGid(gid);
	SYSC_RET1(stack,0);
}

int Syscalls::getgroups(Thread *t,IntrptStackFrame *stack) {
	size_t size = (size_t)SYSC_ARG1(stack);
	gid_t *list = (gid_t*)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();
	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)list,sizeof(gid_t) * size)))
		SYSC_ERROR(stack,-EFAULT);

	size = Groups::get(pid,list,size);
	SYSC_RET1(stack,size);
}

int Syscalls::setgroups(Thread *t,IntrptStackFrame *stack) {
	size_t size = (size_t)SYSC_ARG1(stack);
	const gid_t *list = (const gid_t*)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();
	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)list,sizeof(gid_t) * size)))
		SYSC_ERROR(stack,-EFAULT);

	if(EXPECT_FALSE(!Groups::set(pid,size,list)))
		SYSC_ERROR(stack,-ENOMEM);
	SYSC_RET1(stack,0);
}

int Syscalls::isingroup(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	pid_t pid = (pid_t)SYSC_ARG1(stack);
	gid_t gid = (gid_t)SYSC_ARG2(stack);
	SYSC_RET1(stack,Groups::contains(pid,gid));
}

int Syscalls::fork(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	int res = Proc::clone(0);
	/* error? */
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::waitchild(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	/* better work on a copy in kernel memory. it's not worth the trouble here... */
	Proc::ExitState kstate;
	Proc::ExitState *state = (Proc::ExitState*)SYSC_ARG1(stack);
	if(EXPECT_FALSE(state != NULL && !PageDir::isInUserSpace((uintptr_t)state,sizeof(Proc::ExitState))))
		SYSC_ERROR(stack,-EFAULT);

	int res = Proc::waitChild(&kstate);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	if(state)
		UserAccess::write(state,&kstate,sizeof(kstate));
	SYSC_RET1(stack,0);
}

int Syscalls::exec(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	char pathSave[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	const char *const *args = (const char *const *)SYSC_ARG2(stack);
	const char *const *env = (const char *const *)SYSC_ARG3(stack);
	if(EXPECT_FALSE(!copyPath(pathSave,sizeof(pathSave),path)))
		SYSC_ERROR(stack,-EFAULT);

	int res = Proc::exec(pathSave,args,env);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
