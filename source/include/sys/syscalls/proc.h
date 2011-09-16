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

#ifndef SYSCALLS_PROC_H_
#define SYSCALLS_PROC_H_

#include <sys/common.h>
#include <sys/task/thread.h>
#include <sys/intrpt.h>

#ifdef __i386__
#include <sys/arch/i586/syscalls/proc.h>
#endif

int sysc_getpid(sThread *t,sIntrptStackFrame *stack);
int sysc_getppid(sThread *t,sIntrptStackFrame *stack);
int sysc_getuid(sThread *t,sIntrptStackFrame *stack);
int sysc_setuid(sThread *t,sIntrptStackFrame *stack);
int sysc_geteuid(sThread *t,sIntrptStackFrame *stack);
int sysc_seteuid(sThread *t,sIntrptStackFrame *stack);
int sysc_getgid(sThread *t,sIntrptStackFrame *stack);
int sysc_setgid(sThread *t,sIntrptStackFrame *stack);
int sysc_getegid(sThread *t,sIntrptStackFrame *stack);
int sysc_setegid(sThread *t,sIntrptStackFrame *stack);
int sysc_getgroups(sThread *t,sIntrptStackFrame *stack);
int sysc_setgroups(sThread *t,sIntrptStackFrame *stack);
int sysc_isingroup(sThread *t,sIntrptStackFrame *stack);
int sysc_fork(sThread *t,sIntrptStackFrame *stack);
int sysc_waitChild(sThread *t,sIntrptStackFrame *stack);
int sysc_exec(sThread *t,sIntrptStackFrame *stack);
int sysc_getenvito(sThread *t,sIntrptStackFrame *stack);
int sysc_getenvto(sThread *t,sIntrptStackFrame *stack);
int sysc_setenv(sThread *t,sIntrptStackFrame *stack);

#endif /* SYSCALLS_PROC_H_ */
