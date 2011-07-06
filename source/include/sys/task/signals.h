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

#ifndef SIGNALS_H_
#define SIGNALS_H_

#include <sys/common.h>

#define SIG_COUNT			20

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
#define SIG_THREAD_DIED		5						/* sent to the proc in which the thread died */
#define SIG_PIPE_CLOSED		6						/* sent to the pipe-writer when reader died */
#define SIG_CHILD_TERM		7						/* sent to parent-proc */
#define SIG_INTRPT			8						/* used to interrupt a process; used by shell */
#define SIG_INTRPT_TIMER	9						/* timer-interrupt */
#define SIG_INTRPT_KB		10						/* keyboard-interrupt */
#define SIG_INTRPT_COM1		11						/* com1-interrupt */
#define SIG_INTRPT_COM2		12						/* com2-interrupt */
#define SIG_INTRPT_FLOPPY	13						/* floppy-interrupt */
#define SIG_INTRPT_CMOS		14						/* cmos-interrupt */
#define SIG_INTRPT_ATA1		15						/* ata1-interrupt */
#define SIG_INTRPT_ATA2		16						/* ata2-interrupt */
#define SIG_INTRPT_MOUSE	17						/* mouse-interrupt */
#define SIG_USR1			18						/* can be used for everything */
#define SIG_USR2			19						/* can be used for everything */

/* signal-handler-signature */
typedef void (*fSignal)(int);

/**
 * Initializes the signal-handling
 */
void sig_init(void);

/**
 * Checks whether we can handle the given signal
 *
 * @param signal the signal
 * @return true if so
 */
bool sig_canHandle(sig_t signal);

/**
 * Checks whether the given signal can be send by user-programs
 *
 * @param signal the signal
 * @return true if so
 */
bool sig_canSend(sig_t signal);

/**
 * Sets the given signal-handler for <signal>
 *
 * @param tid the thread-id
 * @param signal the signal
 * @param func the handler-function
 * @return 0 on success
 */
int sig_setHandler(tid_t tid,sig_t signal,fSignal func);

/**
 * Removes the signal-handler for <signal>
 *
 * @param tid the thread-id
 * @param signal the signal
 */
void sig_unsetHandler(tid_t tid,sig_t signal);

/**
 * Removes all handler for the given thread
 *
 * @param tid the thread-id
 */
void sig_removeHandlerFor(tid_t tid);

/**
 * Clones all handler of <parent> for <child>.
 *
 * @param parent the parent-thread-id
 * @param child the child-thread-id
 */
void sig_cloneHandler(tid_t parent,tid_t child);

/**
 * Checks whether there is any signal to handle. If so <sig> and <tid> will be set
 * to the signal to handle.
 *
 * @param sig the signal (will be set on success)
 * @param tid the thread-id (will be set on success)
 * @return true if there is a signal
 */
bool sig_hasSignal(sig_t *sig,tid_t *tid);

/**
 * Checks whether <tid> has a signal
 *
 * @param tid the thread-id
 * @return true if so
 */
bool sig_hasSignalFor(tid_t tid);

/**
 * Adds the given signal for the given process
 *
 * @param pid the process-id
 * @param signal the signal
 * @return true if we should directly switch to the process (handle the signal) or false
 * 	if the process is active and we should do this later
 */
void sig_addSignalFor(pid_t pid,sig_t signal);

/**
 * Adds the given signal to all threads that have announced a handler for it
 *
 * @param signal the signal
 * @return whether the signal has been delivered to somebody
 */
bool sig_addSignal(sig_t signal);

/**
 * Starts handling the given signal. That means the signal will be marked as "active" until
 * sig_ackHandling() will be called.
 *
 * @param tid the thread-id
 * @param signal the signal
 * @return the handler-function
 */
fSignal sig_startHandling(tid_t tid,sig_t signal);

/**
 * Acknoledges the current signal with given thread (marks handling as finished)
 *
 * @param tid the thread-id
 * @return the handled signal
 */
sig_t sig_ackHandling(tid_t tid);

/**
 * @return the total number of announced handlers
 */
size_t sig_dbg_getHandlerCount(void);

/**
 * @param signal the signal-number
 * @return the name of the given signal
 */
const char *sig_dbg_getName(sig_t signal);

/**
 * Prints all announced signal-handlers
 */
void sig_print(void);

#endif /* SIGNALS_H_ */
