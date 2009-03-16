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
typedef void (*fSigHandler)(tSig sigNo);

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

#endif /* SIGNALS_H_ */
