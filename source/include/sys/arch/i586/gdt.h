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

#ifndef GDT_H_
#define GDT_H_

#include <sys/common.h>

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
 * Initializes the TLS-segment for the given TLS-region
 */
void gdt_setTLS(uintptr_t tlsAddr,size_t tlsSize);

/**
 * Sets the stack-pointer for the TSS
 *
 * @param isVM86 whether the current task is a Vm86-task
 */
void tss_setStackPtr(bool isVM86);

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
 */
void tss_setIOMap(const uint8_t *ioMap);

/**
 * Removes the io-map from the TSS so that an exception will be raised if a user-process
 * access a port
 */
void tss_removeIOMap(void);

/**
 * Prints the GDT
 */
void gdt_dbg_print(void);

#endif /*GDT_H_*/
