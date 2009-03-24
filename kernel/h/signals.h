/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef SIGNALS_H_
#define SIGNALS_H_

#include "common.h"

#define SIG_COUNT			14

/* the signals */
#define SIG_KILL			0
#define SIG_TERM			1
#define SIG_ILL_INSTR		2
#define SIG_SEGFAULT		3
#define SIG_PROC_DIED		4
#define SIG_INTRPT			5
#define SIG_INTRPT_TIMER	6
#define SIG_INTRPT_KB		7
#define SIG_INTRPT_COM1		8
#define SIG_INTRPT_COM2		9
#define SIG_INTRPT_FLOPPY	10
#define SIG_INTRPT_CMOS		11
#define SIG_INTRPT_ATA1		12
#define SIG_INTRPT_ATA2		13

/* signal-handler-signature */
typedef void (*fSigHandler)(tSig sigNo,u32 data);

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
 * @param pid the process-id
 * @param signal the signal
 * @param func the handler-function
 * @return 0 if successfull
 */
s32 sig_setHandler(tPid pid,tSig signal,fSigHandler func);

/**
 * Removes the signal-handler for <signal>
 *
 * @param pid the process-id
 * @param signal the signal
 */
void sig_unsetHandler(tPid pid,tSig signal);

/**
 * Removes all handler for the given process
 *
 * @param pid the process-id
 */
void sig_removeHandlerFor(tPid pid);

/**
 * Checks wether there is any signal to handle. If so <sig>,<pid> and <data> will be set
 * to the signal to handle.
 *
 * @param sig the signal (will be set on success)
 * @param pid the process-id (will be set on success)
 * @param data the data to send (will be set on success)
 * @return true if there is a signal
 */
bool sig_hasSignal(tSig *sig,tPid *pid,u32 *data);

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
 * Adds the given signal to all processes that have announced a handler for it
 *
 * @param signal the signal
 * @param data the data to send
 * @return the process-id to which we should switch now or INVALID_PID if we should do this later
 */
tPid sig_addSignal(tSig signal,u32 data);

/**
 * Starts handling the given signal. That means the signal will be marked as "active" until
 * sig_ackHandling() will be called.
 *
 * @param pid the process-id
 * @param signal the signal
 * @return the handler-function or NULL
 */
fSigHandler sig_startHandling(tPid pid,tSig signal);

/**
 * Acknoledges the given signal (marks handling as finished)
 *
 * @param pid the process-id
 * @param signal the signal
 */
void sig_ackHandling(tPid pid,tSig signal);


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
