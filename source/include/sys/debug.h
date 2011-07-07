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

#ifndef SYS_DEBUG_H_
#define SYS_DEBUG_H_

#include <sys/common.h>

#ifdef __eco32__
#include <sys/arch/eco32/debug.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/debug.h>
#endif

/**
 * Starts the timer
 */
void dbg_startTimer(void);

/**
 * Stops the timer and prints the number of clock-cycles done until startTimer()
 */
void dbg_stopTimer(const char *prefix);

#endif /* SYS_DEBUG_H_ */
