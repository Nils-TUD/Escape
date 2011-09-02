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

#ifndef SIGNAL_H_
#define SIGNAL_H_

#include <esc/common.h>

#define SIG_COUNT			20

/* special signal-handler-addresses */
#define SIG_IGN				((fSignal)-3)			/* ignore signal */
#define SIG_DFL				((fSignal)-2)			/* reset to default behaviour */
#define SIG_ERR				((fSignal)-1)			/* error-return */

/* the signals */
#define SIG_RET				-1						/* used to tell the kernel the addr of sigRet */
#define SIG_KILL			0						/* kills a proc; not catchable */
#define SIG_TERM			1						/* terminates a proc; catchable */
#define SIG_ILL_INSTR		2						/* TODO atm unused */
#define SIG_SEGFAULT		3						/* TODO atm unused */
#define SIG_PROC_DIED		4						/* TODO remove */
#define SIG_PIPE_CLOSED		5						/* sent to the pipe-writer when reader died */
#define SIG_CHILD_TERM		6						/* sent to parent-proc */
#define SIG_INTRPT			7						/* used to interrupt a process; used by shell */
#define SIG_INTRPT_TIMER	8						/* timer-interrupt */
#define SIG_INTRPT_KB		9						/* keyboard-interrupt */
#define SIG_INTRPT_COM1		10						/* com1-interrupt */
#define SIG_INTRPT_COM2		11						/* com2-interrupt */
#define SIG_INTRPT_FLOPPY	12						/* floppy-interrupt */
#define SIG_INTRPT_CMOS		13						/* cmos-interrupt */
#define SIG_INTRPT_ATA1		14						/* ata1-interrupt */
#define SIG_INTRPT_ATA2		15						/* ata2-interrupt */
#define SIG_INTRPT_MOUSE	16						/* mouse-interrupt */
#define SIG_ALARM			17						/* for alarm() */
#define SIG_USR1			18						/* can be used for everything */
#define SIG_USR2			19						/* can be used for everything */

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
 * Sets a handler-function for a specific signal
 *
 * @param signal the signal-number
 * @param handler the handler-function
 * @return 0 on success
 */
int setSigHandler(sig_t signal,fSignal handler) A_CHECKRET;

/**
 * Sends the given signal to all process (that have announced a handler)
 *
 * @param signal the signal
 * @return 0 on success
 */
int sendSignal(sig_t signal) A_CHECKRET;

/**
 * Sends the given signal to given process (interrupts can't be sended)
 *
 * @param pid the process-id
 * @param signal the signal
 * @return 0 on success
 */
int sendSignalTo(pid_t pid,sig_t signal) A_CHECKRET;

/**
 * The  signal  system call installs a new signal handler for#
 * signal signum.  The signal handler is set to handler which
 * may be a user specified function, or one of the following:
 * 	SIG_IGN - Ignore the signal.
 * 	SIG_DFL - Reset the signal to its default behavior.
 *
 * @param sig the signal
 * @param handler the new handler-function
 * @return the previous handler-function
 */
fSignal signal(int sig,fSignal handler);

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

#endif /* SIGNAL_H_ */
