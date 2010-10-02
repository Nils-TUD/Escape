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

#ifndef SYSCALLS_THREAD_H_
#define SYSCALLS_THREAD_H_

#include <sys/machine/intrpt.h>

/**
 * Returns the tid of the current thread
 *
 * @return tTid the thread-id
 */
void sysc_gettid(sIntrptStackFrame *stack);

/**
 * @return u32 the number of threads of the current process
 */
void sysc_getThreadCount(sIntrptStackFrame *stack);

/**
 * Starts a new thread
 *
 * @param entryPoint the entry-point
 * @param void* argument
 * @return tTid the new thread-id or < 0
 */
void sysc_startThread(sIntrptStackFrame *stack);

/**
 * Destroys the process and issues a context-switch
 *
 * @param u32 the exit-code
 */
void sysc_exit(sIntrptStackFrame *stack);

/**
 * Returns the cpu-cycles for the current thread
 *
 * @return u64 the cpu-cycles
 */
void sysc_getCycles(sIntrptStackFrame *stack);

/**
 * Blocks the process for a given number of milliseconds
 *
 * @param u32 the number of msecs
 * @return s32 0 on success or a negative error-code
 */
void sysc_sleep(sIntrptStackFrame *stack);

/**
 * Releases the CPU (reschedule)
 */
void sysc_yield(sIntrptStackFrame *stack);

/**
 * Blocks the thread until one of the given events occurrs
 *
 * @param sWaitObject* the objects to wait for
 * @param u32 the number of objects
 * @return s32 0 on success
 */
void sysc_wait(sIntrptStackFrame *stack);

/**
 * First it releases the specified lock. After that it blocks the thread until one of the
 * given events occurrs. This gives user-threads the chance to ensure that a notify() arrives
 * AFTER a thread has called wait. So they can do:
 * thread1:
 *  lock(...);
 *  ...
 *  waitUnlock(...);
 *
 * thread2:
 *  lock(...);
 *  notify(thread1,...);
 *  unlock(...);
 *
 * @param sWaitObject* the objects to wait for
 * @param u32 the number of objects
 * @param u32 ident the ident to unlock
 * @param bool global whether the ident is global
 * @return s32 0 on success
 */
void sysc_waitUnlock(sIntrptStackFrame *stack);

/**
 * Notifies the given thread about the given events. If it was waiting for them, it will be
 * waked up.
 *
 * @param tTid the thread-id
 * @param u16 events the events to notify about
 * @return s32 0 on success
 */
void sysc_notify(sIntrptStackFrame *stack);

/**
 * Aquire a lock; wait until its unlocked, if necessary
 *
 * @param ident the lock-ident
 * @return s32 0 on success
 */
void sysc_lock(sIntrptStackFrame *stack);

/**
 * Releases a lock
 *
 * @param ident the lock-ident
 * @return s32 0 on success
 */
void sysc_unlock(sIntrptStackFrame *stack);

/**
 * Joins a thread, i.e. it waits until a thread with given tid has died (from the own process)
 *
 * @param tTid the thread-id (0 = wait until all other threads died)
 * @return 0 on success
 */
void sysc_join(sIntrptStackFrame *stack);

/**
 * Suspends a thread from the own process. That means it is blocked until resume() is called.
 *
 * @param tTid the thread-id
 * @return 0 on success
 */
void sysc_suspend(sIntrptStackFrame *stack);

/**
 * Resumes a thread from the own process that has been suspended previously
 *
 * @param tTid the thread-id
 * @return 0 on success
 */
void sysc_resume(sIntrptStackFrame *stack);

#endif /* SYSCALLS_THREAD_H_ */
