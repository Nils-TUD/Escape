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
#include <signal.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define WEXIT_SIG_MASK			0xFF00
#define WEXIT_STATUS_MASK		0xFF

#define WTERMSIG(stat_val)		(((stat_val) & WEXIT_SIG_MASK) >> 8)
#define WIFEXITED(stat_val)		(WTERMSIG(stat_val) == SIG_COUNT)
#define WEXITSTATUS(stat_val)	((stat_val) & WEXIT_STATUS_MASK)
#define WIFSIGNALED(stat_val)	(WTERMSIG(stat_val) != SIG_COUNT)
#define WIFSTOPPED(stat_val)	0
#define WSTOPSIG(stat_val)		0
#define WIFCONTINUED(stat_val)	0

enum {
	WNOHANG	= 1,
};

typedef struct {
	pid_t pid;
	/* the signal that killed the process (SIG_COUNT if none) */
	int signal;
	/* exit-code the process gave us via exit() */
	int exitCode;
	/* memory it has used */
	ulong ownFrames;
	ulong sharedFrames;
	ulong swapped;
	/* other stats */
	uint64_t runtime;
	ulong schedCount;
	ulong syscalls;
	ulong migrations;
} sExitState;

/**
 * Waits until a child process terminates and obtains status information from one of the
 * child-process. If pid is not -1, it waits until the child with that pid exits.
 *
 * If WNOHANG is specified and no child has exited, waitpid() returns 0 immediately.
 *
 * @param pid the child to wait for (-1 = any child)
 * @param stat_loc will be set to the status value
 * @param options 0 or WNOHANG
 * @return the pid of the child process or a negative error value
 */
pid_t waitpid(pid_t pid,int *stat_loc,int options);

/**
 * Waits until a child process terminates and obtains status information from one of the
 * child-process.
 *
 * @param stat_loc will be set to the status value
 * @return the pid of the child process or a negative error value
 */
static inline pid_t wait(int *stat_loc) {
	return waitpid(-1,stat_loc,0);
}

/**
 * Waits until a child or a specific one terminates and stores information about it into <state>.
 * Note that a child-process is required and only one thread can wait for a child-process!
 * You may get interrupted by a signal (and may want to call waitchild() again in this case). If so
 * you get -EINTR as return-value (and errno will be set).
 *
 * If WNOHANG is specified and no child has exited, waitpid() returns 0 immediately. To check
 * whether that happened, set state->pid to 0 before the call and check if it's still 0 afterwards.
 *
 * @param state the exit-state (may be NULL)
 * @param pid the pid of the child to wait for (-1 = any child)
 * @param options 0 or WNOHANG
 * @return 0 on success
 */
static inline int waitchild(sExitState *state,int pid,int options) {
	return syscall3(SYSCALL_WAITCHILD,(ulong)state,pid,options);
}

#if defined(__cplusplus)
}
#endif
