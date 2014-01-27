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

#include <esc/common.h>
#include <esc/syscalls.h>

#define RW_READ		0
#define RW_WRITE	1

typedef struct {
	int sem;
	long value;
} tUserSem;

typedef struct {
	// semaphore for blocking
	int sem;
	// -1 if somebody writes, >0 if we're reading
	volatile int count;
	// the number of waiters
	int waits;
	// for accessing the members of this structure
	tUserSem mutex;
} tRWLock;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a process-local semaphore with initial value <value>. This will be inherited to child-
 * processes which will then share the same semaphore, so that they can synchronize. Process-local
 * semaphores don't survive execs, though.
 *
 * @param value the initial value
 * @return the sem-id or the error-code
 */
static inline int semcrt(uint value) {
	return syscall1(SYSCALL_SEMCRT,value);
}

/**
 * Performs the down-operation on the given semaphore, i.e. decrements it.
 * If the value is <= 0, it blocks until semup() is called.
 *
 * @param id the sem-id
 */
static inline int semdown(int id) {
	return syscall2(SYSCALL_SEMOP,id,-1);
}

/**
 * Performs the up-operation on the given semaphore, i.e. increments it.
 *
 * @param id the sem-id
 */
static inline int semup(int id) {
	return syscall2(SYSCALL_SEMOP,id,+1);
}

/**
 * Destroys the given semaphore.
 *
 * @param id the sem-id
 */
static inline void semdestr(int id) {
	syscall1(SYSCALL_SEMDESTROY,id);
}

/**
 * Initializes a user-semaphore, which is optimized for the non-contention case. In most cases, the
 * up/down operation will only perform a atomic increment/decrement and only in the contention-case
 * a kernel-semaphore is used for blocking. That is, the implementation is based on sem*.
 *
 * @param sem the semaphore
 * @param val the initial value
 * @return >= 0 on success
 */
static inline int usemcrt(tUserSem *sem,long val);

/**
 * Performs the down-operation on the given user-semaphore, i.e. decrements it.
 * If the value is <= 0, it blocks until usemup() is called.
 *
 * @param sem the semaphore
 */
static inline void usemdown(tUserSem *sem);

/**
 * Performs the up-operation on the given user-semaphore, i.e. increments it.
 *
 * @param sem the semaphore
 */
static inline void usemup(tUserSem *sem);

/**
 * Destroys the given user-semaphore.
 *
 * @param sem the semaphore
 */
static inline void usemdestr(tUserSem *sem);

/**
 * Creates a readers-writer lock. Multiply readers can use the lock in parallel, while a writer
 * always has to be alone.
 *
 * @param l the lock
 * @return 0 on success
 */
int rwcrt(tRWLock *l);

/**
 * Requests the given readers-writer-lock for <op>.
 *
 * @param l the lock
 * @param op the operation (RW_READ or RW_WRITE)
 */
void rwreq(tRWLock *l,int op);

/**
 * Releases the given readers-writer-lock again for <op>.
 *
 * @param l the lock
 * @param op the operation (RW_READ or RW_WRITE)
 */
void rwrel(tRWLock *l,int op);

/**
 * Destroys the readers-writer-lock.
 *
 * @param l the lock
 */
void rwdestr(tRWLock *l);

#ifdef __cplusplus
}
#endif

#ifdef __i386__
#	include <esc/arch/i586/sync.h>
#endif
#ifdef __eco32__
#	include <esc/arch/eco32/sync.h>
#endif
#ifdef __mmix__
#	include <esc/arch/mmix/sync.h>
#endif
