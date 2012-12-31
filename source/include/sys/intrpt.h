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
 * Initializes the interrupts
 */
void intrpt_init(void);

/**
 * @return the total number of interrupts so far
 */
size_t intrpt_getCount(void);

/**
 * @param irq the interrupt-number
 * @return the vector for the given interrupt
 */
uint8_t intrpt_getVectorFor(uint8_t irq);

/**
 * Handles an interrupt
 *
 * @param number the interrupt-number
 */
void intrpt_handler(sIntrptStackFrame *stack);

/**
 * Prints the given interrupt-stack
 *
 * @param stack the interrupt-stack
 */
void intrpt_printStackFrame(const sIntrptStackFrame *stack);
