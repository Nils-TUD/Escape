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

#include <esc/common.h>

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
 * @return 0 on success
 */
s32 ackSignal(void);

/**
 * Sends the given signal to all process (that have announced a handler)
 *
 * @param signal the signal
 * @param data the data to send
 * @return 0 on success
 */
s32 sendSignal(tSig signal,u32 data);

/**
 * Sends the given signal to given process (interrupts can't be sended)
 *
 * @param pid the process-id
 * @param signal the signal
 * @param data the data to send
 * @return 0 on success
 */
s32 sendSignalTo(tPid pid,tSig signal,u32 data);

#ifdef __cplusplus
}
#endif

#endif /* SIGNALS_H_ */
