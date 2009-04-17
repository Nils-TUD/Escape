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

#include "common.h"

/* the size of the io-map (in bits) */
#define IO_MAP_SIZE				0xFFFF

/**
 * Inits the GDT
 */
void gdt_init(void);

/**
 * Removes the io-map from the TSS so that an exception will be raised if a user-process
 * access a port
 */
void tss_removeIOMap(void);

/**
 * Checks wether the io-map is set
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
void tss_setIOMap(u8 *ioMap);


#if DEBUGGING

/**
 * Prints the GDT
 */
void gdt_dbg_print(void);

#endif

#endif /*GDT_H_*/
