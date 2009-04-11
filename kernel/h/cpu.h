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

/**
 * @return the timestamp-counter value
 */
extern u64 cpu_rdtsc(void);

/**
 * @return the value of the CR0 register
 */
extern u32 cpu_getCR0(void);

/**
 * @return the value of the CR2 register, that means the linear address that caused a page-fault
 */
extern u32 cpu_getCR2(void);

/**
 * @return the value of the CR3 register, that means the physical address of the page-directory
 */
extern u32 cpu_getCR3(void);

/**
 * @return the value of the CR4 register
 */
extern u32 cpu_getCR4(void);

#endif /*CPU_H_*/
