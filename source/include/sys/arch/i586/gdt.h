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
#include <sys/task/thread.h>

/* the size of the io-map (in bits) */
#define IO_MAP_SIZE				0xFFFF

/* the segment-indices in the gdt */
#define SEGSEL_GDTI_KCODE		(1 << 3)
#define SEGSEL_GDTI_KDATA		(2 << 3)
#define SEGSEL_GDTI_UCODE		(3 << 3)
#define SEGSEL_GDTI_UDATA		(4 << 3)
#define SEGSEL_GDTI_UTLS		(5 << 3)

/* the table-indicators */
#define SEGSEL_TI_GDT			0x00
#define SEGSEL_TI_LDT			0x04

/* the requested privilege levels */
#define SEGSEL_RPL_USER			0x03
#define SEGSEL_RPL_KERNEL		0x00

/**
 * Inits the GDT
 */
void gdt_init(void);

/**
 * Finishes the initialization for the bootstrap processor
 */
void gdt_init_bsp(void);

/**
 * Finishes the initialization for an application processor
 */
void gdt_init_ap(void);

/**
 * @return the current cpu-id
 */
cpuid_t gdt_getCPUId(void);

/**
 * Sets the running thread for the given CPU
 *
 * @param id the cpu-id
 * @param t the thread
 */
void gdt_setRunning(cpuid_t id,Thread *t);

/**
 * Prepares the run of the given thread, i.e. sets the stack-pointer for interrupts, removes
 * the I/O map and sets the TLS-register.
 *
 * @param old the old thread (may be NULL)
 * @param n the thread to run
 * @return the cpu-id for the new thread
 */
cpuid_t gdt_prepareRun(Thread *old,Thread *n);

/**
 * Checks whether the io-map is set
 *
 * @return true if so
 */
bool tss_ioMapPresent(void);

/**
 * Sets the given io-map. That means it copies IO_MAP_SIZE / 8 bytes from the given address
 * into the TSS
 *
 * @param ioMap the io-map to set
 * @param forceCpy whether to force a copy of the given map (necessary if it has changed)
 */
void tss_setIOMap(const uint8_t *ioMap,bool forceCpy);

/**
 * Prints the GDT
 */
void gdt_print(void);
