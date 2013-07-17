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

int sysc_gettid(Thread *t,sIntrptStackFrame *stack);
int sysc_getthreadcnt(Thread *t,sIntrptStackFrame *stack);
int sysc_startthread(Thread *t,sIntrptStackFrame *stack);
int sysc_exit(Thread *t,sIntrptStackFrame *stack);
int sysc_getcycles(Thread *t,sIntrptStackFrame *stack);
int sysc_alarm(Thread *t,sIntrptStackFrame *stack);
int sysc_sleep(Thread *t,sIntrptStackFrame *stack);
int sysc_yield(Thread *t,sIntrptStackFrame *stack);
int sysc_wait(Thread *t,sIntrptStackFrame *stack);
int sysc_waitunlock(Thread *t,sIntrptStackFrame *stack);
int sysc_notify(Thread *t,sIntrptStackFrame *stack);
int sysc_lock(Thread *t,sIntrptStackFrame *stack);
int sysc_unlock(Thread *t,sIntrptStackFrame *stack);
int sysc_join(Thread *t,sIntrptStackFrame *stack);
int sysc_suspend(Thread *t,sIntrptStackFrame *stack);
int sysc_resume(Thread *t,sIntrptStackFrame *stack);
