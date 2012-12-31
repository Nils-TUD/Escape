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
#include <esc/sllist.h>

#define SIG_COUNT			19

#define SIG_IGN				((fSignal)-3)			/* ignore signal */
#define SIG_DFL				((fSignal)-2)			/* reset to default behaviour */
#define SIG_ERR				((fSignal)-1)			/* error-return */

/* the signals */
#define SIG_RET				-1						/* used to tell the kernel the addr of sigRet */
#define SIG_KILL			0						/* kills a proc; not catchable */
#define SIG_TERM			1						/* terminates a proc; catchable */
#define SIG_ILL_INSTR		2						/* TODO atm unused */
#define SIG_SEGFAULT		3						/* segmentation fault */
#define SIG_PIPE_CLOSED		4						/* sent to the pipe-writer when reader died */
#define SIG_CHILD_TERM		5						/* sent to parent-proc */
#define SIG_INTRPT			6						/* used to interrupt a process; used by shell */
#define SIG_INTRPT_TIMER	7						/* timer-interrupt */
#define SIG_INTRPT_KB		8						/* keyboard-interrupt */
#define SIG_INTRPT_COM1		9						/* com1-interrupt */
#define SIG_INTRPT_COM2		10						/* com2-interrupt */
#define SIG_INTRPT_FLOPPY	11						/* floppy-interrupt */
#define SIG_INTRPT_CMOS		12						/* cmos-interrupt */
#define SIG_INTRPT_ATA1		13						/* ata1-interrupt */
#define SIG_INTRPT_ATA2		14						/* ata2-interrupt */
#define SIG_INTRPT_MOUSE	15						/* mouse-interrupt */
#define SIG_ALARM			16						/* for alarm() */
#define SIG_USR1			17						/* can be used for everything */
#define SIG_USR2			18						/* can be used for everything */

#define SIG_CHECK_CUR		0
#define SIG_CHECK_OTHER		1
#define SIG_CHECK_NO		2

/* signal-handler-signature */
typedef void (*fSignal)(int);

typedef struct sPendingSig {
	int sig;
	struct sPendingSig *next;
} sPendingSig;

typedef struct {
	sPendingSig *first;
	sPendingSig *last;
	size_t count;
} sPendingQueue;

typedef struct {
	/* list of pending signals */
	sPendingQueue pending;
	/* signal handler */
	fSignal handler[SIG_COUNT];
	/* the signal that the thread is currently handling (if > 0) */
	int currentSignal;
	/* the signal that the thread should handle now */
	int deliveredSignal;
} sSignals;

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
bool sig_canHandle(int signal);

/**
 * Checks whether the given signal can be send by user-programs
 *
 * @param signal the signal
 * @return true if so
 */
bool sig_canSend(int signal);

/**
 * @param sig the signal
 * @return whether the given signal is a fatal one, i.e. should kill the process
 */
bool sig_isFatal(int sig);

/**
 * Sets the given signal-handler for <signal>
 *
 * @param tid the thread-id
 * @param signal the signal
 * @param func the handler-function
 * @param old will be set to the old handler
 * @return 0 on success
 */
int sig_setHandler(tid_t tid,int signal,fSignal func,fSignal *old);

/**
 * Removes the signal-handler for <signal>
 *
 * @param tid the thread-id
 * @param signal the signal
 * @return the old handler
 */
fSignal sig_unsetHandler(tid_t tid,int signal);

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
 * Checks whether <tid> has a signal
 *
 * @param tid the thread-id
 * @return true if so
 */
bool sig_hasSignalFor(tid_t tid);

/**
 * Checks whether a signal should be handled. If the current thread has a signal, it returns
 * SIG_CHECK_CUR and sets *sig and *handler correspondingly. If another thread has a signal, it
 * delivers it and returns SIG_CHECK_OTHER. Otherwise, it returns SIG_CHECK_NO.
 *
 * @param tid the thread-id of the current thread
 * @param sig will be set to the signal to handle, if the current thread has a signal
 * @param handler will be set to the handler, if the current thread has a signal
 * @return SIG_CHECK_* the result
 */
int sig_checkAndStart(tid_t tid,int *sig,fSignal *handler);

/**
 * Adds the given signal for the given thread
 *
 * @param tid the thread-id
 * @param signal the signal
 * @return true if the signal has been added
 */
bool sig_addSignalFor(tid_t tid,int signal);

/**
 * Adds the given signal to all threads that have announced a handler for it
 *
 * @param signal the signal
 * @return whether the signal has been delivered to somebody
 */
bool sig_addSignal(int signal);

/**
 * Acknoledges the current signal with given thread (marks handling as finished)
 *
 * @param tid the thread-id
 * @return the handled signal
 */
int sig_ackHandling(tid_t tid);

/**
 * @return the total number of announced handlers
 */
size_t sig_dbg_getHandlerCount(void);

/**
 * @param signal the signal-number
 * @return the name of the given signal
 */
const char *sig_dbg_getName(int signal);

/**
 * Prints all announced signal-handlers
 */
void sig_print(void);
