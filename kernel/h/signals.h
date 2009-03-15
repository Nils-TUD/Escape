/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef SIGNALS_H_
#define SIGNALS_H_

#include "common.h"

#define SIG_COUNT			12

/* the signals */
#define SIG_KILL			0
#define SIG_TERM			1
#define SIG_ILL_INSTR		2
#define SIG_SEGFAULT		3
#define SIG_INTRPT_TIMER	4
#define SIG_INTRPT_KB		5
#define SIG_INTRPT_COM1		6
#define SIG_INTRPT_COM2		7
#define SIG_INTRPT_FLOPPY	8
#define SIG_INTRPT_CMOS		9
#define SIG_INTRPT_ATA1		10
#define SIG_INTRPT_ATA2		11

/* signal-handler-signature */
typedef void (*fSigHandler)(u8 sigNo);

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
bool sig_canHandle(u8 signal);

/**
 * Sets the given signal-handler for <signal>
 *
 * @param pid the process-id
 * @param signal the signal
 * @param func the handler-function
 * @return 0 if successfull
 */
s32 sig_setHandler(tPid pid,u8 signal,fSigHandler func);

/**
 * Removes the signal-handler for <signal>
 *
 * @param pid the process-id
 * @param signal the signal
 */
void sig_unsetHandler(tPid pid,u8 signal);

/**
 * Checks wether there is any signal to handle. If so <sig> and <pid> will be set
 * to the signal to handle.
 *
 * @param sig the signal (will be set on success)
 * @param pid the process-id (will be set on success)
 * @return true if there is a signal
 */
bool sig_hasSignal(u8 *sig,tPid *pid);

/**
 * Adds the given signal for the given process
 *
 * @param pid the process-id
 * @param signal the signal
 * @return true if we should directly switch to the process (handle the signal) or false
 * 	if the process is active and we should do this later
 */
bool sig_addSignalFor(tPid pid,u8 signal);

/**
 * Adds the given signal to all processes that have announced a handler for it
 *
 * @param signal the signal
 * @return the process-id to which we should switch now or INVALID_PID if we should do this later
 */
tPid sig_addSignal(u8 signal);

/**
 * Starts handling the given signal. That means the signal will be marked as "active" until
 * sig_ackHandling() will be called.
 *
 * @param pid the process-id
 * @param signal the signal
 */
void sig_startHandling(tPid pid,u8 signal);

/**
 * Acknoledges the given signal (marks handling as finished)
 *
 * @param pid the process-id
 * @param signal the signal
 */
void sig_ackHandling(tPid pid,u8 signal);

#endif /* SIGNALS_H_ */
