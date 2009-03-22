/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef SIGNALS_H_
#define SIGNALS_H_

#include "common.h"

#define SIG_COUNT			13

/* the signals */
#define SIG_KILL			0
#define SIG_TERM			1
#define SIG_ILL_INSTR		2
#define SIG_SEGFAULT		3
#define SIG_PROC_DIED		4
#define SIG_INTRPT_TIMER	5
#define SIG_INTRPT_KB		6
#define SIG_INTRPT_COM1		7
#define SIG_INTRPT_COM2		8
#define SIG_INTRPT_FLOPPY	9
#define SIG_INTRPT_CMOS		10
#define SIG_INTRPT_ATA1		11
#define SIG_INTRPT_ATA2		12

/* signal-handler-signature */
typedef void (*fSigHandler)(tSig sigNo,u32 data);

/**
 * Sets a handler-function for a specific signal
 *
 * @param signal the signal-number
 * @param handler the handler-function
 * @return 0 on success
 */
s32 setSigHandler(tSig signal,fSigHandler handler);

/**
 * Unsets the handler-function for a specific signal
 *
 * @param signal the signal-number
 * @return 0 on success
 */
s32 unsetSigHandler(tSig signal);

/**
 * Acknoledges that the processing of a signal is finished
 *
 * @param signal the signal-number
 * @return 0 on success
 */
s32 ackSignal(tSig signal);

/**
 * Sends the given signal to given process (interrupts can't be sended)
 *
 * @param pid the process-id
 * @param signal the signal
 * @param data the data to send
 * @return 0 on success
 */
s32 sendSignal(tPid pid,tSig signal,u32 data);

#endif /* SIGNALS_H_ */
