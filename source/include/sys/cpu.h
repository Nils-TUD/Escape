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

#ifndef CPU_H_
#define CPU_H_

#include <esc/common.h>
#include <sys/printf.h>

#ifdef __i386__
#include <sys/arch/i586/cpu.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/cpu.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/cpu.h>
#endif

/**
 * @return the timestamp-counter value
 */
uint64_t cpu_rdtsc(void);

/**
 * Prints information about the used CPU into the given string-buffer
 *
 * @param buf the string-buffer
 */
void cpu_sprintf(sStringBuffer *buf);

/**
 * Prints the CPU-information
 */
void cpu_dbg_print(void);

#endif /* CPU_H_ */
