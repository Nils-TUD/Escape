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

#pragma once

#include <sys/common.h>
#include <sys/task/thread.h>
#include <sys/intrpt.h>

#ifdef __i386__
#include <sys/arch/i586/syscalls/proc.h>
#endif

int sysc_getpid(Thread *t,sIntrptStackFrame *stack);
int sysc_getppid(Thread *t,sIntrptStackFrame *stack);
int sysc_getuid(Thread *t,sIntrptStackFrame *stack);
int sysc_setuid(Thread *t,sIntrptStackFrame *stack);
int sysc_geteuid(Thread *t,sIntrptStackFrame *stack);
int sysc_seteuid(Thread *t,sIntrptStackFrame *stack);
int sysc_getgid(Thread *t,sIntrptStackFrame *stack);
int sysc_setgid(Thread *t,sIntrptStackFrame *stack);
int sysc_getegid(Thread *t,sIntrptStackFrame *stack);
int sysc_setegid(Thread *t,sIntrptStackFrame *stack);
int sysc_getgroups(Thread *t,sIntrptStackFrame *stack);
int sysc_setgroups(Thread *t,sIntrptStackFrame *stack);
int sysc_isingroup(Thread *t,sIntrptStackFrame *stack);
int sysc_fork(Thread *t,sIntrptStackFrame *stack);
int sysc_waitchild(Thread *t,sIntrptStackFrame *stack);
int sysc_exec(Thread *t,sIntrptStackFrame *stack);
int sysc_getenvito(Thread *t,sIntrptStackFrame *stack);
int sysc_getenvto(Thread *t,sIntrptStackFrame *stack);
int sysc_setenv(Thread *t,sIntrptStackFrame *stack);
