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

typedef int	sig_atomic_t;
typedef void (*sighandler_t)(int);

#define SIG_COUNT			11

#define SIG_IGN				((sighandler_t)-2)		/* ignore signal */
#define SIG_ERR				((sighandler_t)-1)		/* error-return */
#define SIG_DFL				((sighandler_t)0)		/* reset to default behaviour */

/* the signals */
#define SIGRET				-1						/* used to tell the kernel the addr of sigRet */
#define SIGKILL				0
#define SIGABRT				SIGKILL					/* kill process in every case */
#define SIGTERM				1						/* kindly ask process to terminate */
#define SIGFPE				2						/* arithmetic operation (div by zero, ...) */
#define SIGILL				3						/* illegal instruction */
#define SIGINT				4						/* interactive attention signal */
#define SIGSEGV				5						/* segmentation fault */
#define SIGCHLD				6						/* sent to parent-proc */
#define SIGALRM				7						/* for alarm() */
#define SIGUSR1				8						/* can be used for everything */
#define SIGUSR2				9						/* can be used for everything */
#define SIGCANCEL			10						/* is sent by cancel() */

#if defined(__cplusplus)
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
static inline sighandler_t signal(int sig,sighandler_t handler) {
	return (sighandler_t)syscall2(SYSCALL_SETSIGH,sig,(ulong)handler);
}

/**
 * Sends the given signal to given process. To do so, write access to /sys/proc/<pid> is required.
 *
 * @param pid the process-id
 * @param signal the signal
 * @return 0 on success
 */
int kill(pid_t pid,int signal);

/**
 * raise()  sends  a  signal  to  the current process.  It is
 * equivalent to kill(getpid(),sig)
 *
 * @param sig the signal
 * @return zero on success
 */
int raise(int sig);

#if defined(__cplusplus)
}
#endif
