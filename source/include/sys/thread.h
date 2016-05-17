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

#pragma once

#include <sys/common.h>
#include <sys/syscalls.h>

#define MAX_STACK_PAGES			128

/* the thread-entry-point-function */
typedef int (*fThreadEntry)(void *arg);

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @return the id of the current thread
 */
static inline tid_t gettid(void) {
	return syscall0(SYSCALL_GETTID);
}

/**
 * Starts a new thread
 *
 * @param entryPoint the entry-point of the thread
 * @param arg the argument to pass (just the pointer)
 * @return the new tid, < 0 if failed
 */
int startthread(fThreadEntry entryPoint,void *arg) A_CHECKRET;

/**
 * The syscall exit
 *
 * @param errorCode the error-code for the parent
 */
A_NORETURN static inline void _exit(int exitCode) {
	syscall1(SYSCALL_EXIT,exitCode);
	A_UNREACHED;
}

/**
 * @return the cpu-cycles of the current thread
 */
static inline uint64_t getcycles(void) {
	uint64_t res;
	syscall1(SYSCALL_GETCYCLES,(ulong)&res);
	return res;
}

/**
 * Releases the CPU (reschedule)
 */
static inline void yield(void) {
	syscall0(SYSCALL_YIELD);
}

/**
 * Notifies the thread in <usecs> microseconds via signal (SIGALRM).
 *
 * @param usecs the number of microseconds
 * @return 0 on success
 */
static inline int ualarm(time_t usecs) {
	return syscall1(SYSCALL_ALARM,usecs);
}

/**
 * Notifies the thread in <secs> seconds via signal (SIGALRM).
 *
 * @param secs the number of milliseconds
 * @return 0 on success
 */
static inline int alarm(uint secs) {
	return ualarm(secs * 1000000);
}

/**
 * Puts the current thread to sleep for <usecs> microseconds. If interrupted, -EINTR
 * is returned.
 *
 * @param usecs the number of microseconds to wait
 * @return 0 on success
 */
static inline int usleep(time_t usecs) {
	return syscall1(SYSCALL_SLEEP,usecs);
}

/**
 * Puts the current thread to sleep for <secs> seconds. If interrupted, -EINTR
 * is returned.
 *
 * @param secs the number of seconds to wait
 * @return 0 on success
 */
static inline int sleep(uint secs) {
	return usleep(secs * 1000000);
}

/**
 * Joins a thread, i.e. it waits until a thread with given tid has died (from the own process).
 * If interrupted by a signal, -EINTR is returned.
 *
 * @param tid the thread-id (0 = wait until all other threads died)
 * @return 0 on success
 */
static inline int join(tid_t tid) {
	return syscall1(SYSCALL_JOIN,tid);
}

#if defined(__cplusplus)
}
#endif
