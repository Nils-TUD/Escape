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

#include <common.h>

#define SIG_COUNT			17

/* the signals */
#define SIG_KILL			0
#define SIG_TERM			1
#define SIG_ILL_INSTR		2
#define SIG_SEGFAULT		3
#define SIG_PROC_DIED		4
#define SIG_THREAD_DIED		5
#define SIG_CHILD_DIED		6
#define SIG_INTRPT			7
#define SIG_INTRPT_TIMER	8
#define SIG_INTRPT_KB		9
#define SIG_INTRPT_COM1		10
#define SIG_INTRPT_COM2		11
#define SIG_INTRPT_FLOPPY	12
#define SIG_INTRPT_CMOS		13
#define SIG_INTRPT_ATA1		14
#define SIG_INTRPT_ATA2		15
#define SIG_INTRPT_MOUSE	16

/* signal-handler-signature */
typedef void (*fSigHandler)(tSig sigNo,u32 data);

/**
 * Inits the signal-handling
 */
void sig_init(void);

/**
 * Checks wether we can handle the given signal
 *
 * @param signal the signal
 * @return true if so
 */
bool sig_canHandle(tSig signal);

/**
 * Checks wether the given signal can be send by user-programs
 *
 * @param signal the signal
 * @return true if so
 */
bool sig_canSend(tSig signal);

/**
 * Sets the given signal-handler for <signal>
 *
 * @param tid the thread-id
 * @param signal the signal
 * @param func the handler-function
 * @return 0 if successfull
 */
s32 sig_setHandler(tTid tid,tSig signal,fSigHandler func);

/**
 * Removes the signal-handler for <signal>
 *
 * @param tid the thread-id
 * @param signal the signal
 */
void sig_unsetHandler(tTid tid,tSig signal);

/**
 * Removes all handler for the given thread
 *
 * @param tid the thread-id
 */
void sig_removeHandlerFor(tTid tid);

/**
 * Checks wether there is any signal to handle. If so <sig>,<tid> and <data> will be set
 * to the signal to handle.
 *
 * @param sig the signal (will be set on success)
 * @param tid the thread-id (will be set on success)
 * @param data the data to send (will be set on success)
 * @return true if there is a signal
 */
bool sig_hasSignal(tSig *sig,tPid *tid,u32 *data);

/**
 * Checks wether <tid> has a signal
 *
 * @param tid the thread-id
 * @return true if so
 */
bool sig_hasSignalFor(tTid tid);

/**
 * Adds the given signal for the given process
 *
 * @param pid the process-id
 * @param signal the signal
 * @param data the data to send
 * @return true if we should directly switch to the process (handle the signal) or false
 * 	if the process is active and we should do this later
 */
bool sig_addSignalFor(tPid pid,tSig signal,u32 data);

/**
 * Adds the given signal to all threads that have announced a handler for it
 *
 * @param signal the signal
 * @param data the data to send
 * @return the thread-id to which we should switch now or INVALID_TID if we should do this later
 */
tTid sig_addSignal(tSig signal,u32 data);

/**
 * Starts handling the given signal. That means the signal will be marked as "active" until
 * sig_ackHandling() will be called.
 *
 * @param tid the thread-id
 * @param signal the signal
 * @return the handler-function or NULL
 */
fSigHandler sig_startHandling(tTid tid,tSig signal);

/**
 * Acknoledges the current signal with given thread (marks handling as finished)
 *
 * @param tid the thread-id
 */
void sig_ackHandling(tTid tid);


#if DEBUGGING

/**
 * @return the total number of announced handlers
 */
u32 sig_dbg_getHandlerCount(void);

/**
 * @param signal the signal-number
 * @return the name of the given signal
 */
const char *sig_dbg_getName(tSig signal);

/**
 * Prints all announced signal-handlers
 */
void sig_dbg_print(void);

#endif

#endif /* SIGNALS_H_ */
