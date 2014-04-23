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

#define SIG_COUNT			10

#define SIG_IGN				((fSignal)-2)			/* ignore signal */
#define SIG_ERR				((fSignal)-1)			/* error-return */
#define SIG_DFL				((fSignal)0)			/* reset to default behaviour */

/* the signals */
#define SIG_RET				-1						/* used to tell the kernel the addr of sigRet */
#define SIG_KILL			0						/* kills a proc; not catchable */
#define SIG_TERM			1						/* terminates a proc; catchable */
#define SIG_ILL_INSTR		2						/* TODO atm unused */
#define SIG_SEGFAULT		3						/* segmentation fault */
#define SIG_PIPE_CLOSED		4						/* sent to the pipe-writer when reader died */
#define SIG_CHILD_TERM		5						/* sent to parent-proc */
#define SIG_INTRPT			6						/* used to interrupt a process; used by shell */
#define SIG_ALARM			7						/* for alarm() */
#define SIG_USR1			8						/* can be used for everything */
#define SIG_USR2			9						/* can be used for everything */

/* standarg signal-names */
#define SIGABRT				SIG_KILL
/* TODO */
#define SIGFPE				-1
#define SIGILL				SIG_ILL_INSTR
#define SIGINT				SIG_INTRPT
#define SIGSEGV				SIG_SEGFAULT
#define SIGTERM				SIG_TERM

/* the signature */
typedef void (*fSignal)(int);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The  signal  system call installs a new signal handler for#
 * signal signum.  The signal handler is set to handler which
 * may be a user specified function, or one of the following:
 * 	SIG_IGN - Ignore the signal.
 * 	SIG_DFL - Reset the signal to its default behavior.
 *
 * @param sig the signal
 * @param handler the new handler-function
 * @return the previous handler-function or SIG_ERR
 */
static inline fSignal signal(int sig,fSignal handler) {
	return (fSignal)syscall2(SYSCALL_SETSIGH,sig,(ulong)handler);
}

/**
 * Sends the given signal to given process
 *
 * @param pid the process-id
 * @param signal the signal
 * @return 0 on success
 */
static inline int kill(pid_t pid,int signal) {
	return syscall2(SYSCALL_SENDSIG,pid,signal);
}

/**
 * raise()  sends  a  signal  to  the current process.  It is
 * equivalent to kill(getpid(),sig)
 *
 * @param sig the signal
 * @return zero on success
 */
int raise(int sig);

#ifdef __cplusplus
}
#endif
