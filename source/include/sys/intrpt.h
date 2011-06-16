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

#ifndef INTRPT_H_
#define INTRPT_H_

#include <sys/common.h>

#ifdef __i386__
#include <sys/arch/i586/intrpt.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/intrpt.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/intrpt.h>
#endif

/**
 * Enables / disables the interrupts
 *
 * @param enabled the new value
 * @return the old value
 */
bool intrpt_setEnabled(bool enabled);

/**
 * Initializes the interrupts
 */
void intrpt_init(void);

/**
 * @return the total number of interrupts so far
 */
size_t intrpt_getCount(void);

/**
 * Handles an interrupt
 *
 * @param number the interrupt-number
 */
void intrpt_handler(sIntrptStackFrame *stack);

#if DEBUGGING

/**
 * Prints the given interrupt-stack
 *
 * @param stack the interrupt-stack
 */
void intrpt_dbg_printStackFrame(const sIntrptStackFrame *stack);

#endif

#endif /*INTRPT_H_*/
