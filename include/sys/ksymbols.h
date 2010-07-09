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

#ifndef KSYMBOLS_H_
#define KSYMBOLS_H_

#include <sys/common.h>

/* Represents one symbol (address -> name) */
typedef struct {
	u32 address;
	const char *funcName;
} sSymbol;

/**
 * Prints all kernel-symbols
 */
void ksym_print(void);

/**
 * Searches for the highest address lower than the given one in the symbol-table. In other
 * words: The name of the function to which the given address belongs
 *
 * @param address the address
 * @return the pointer to the symbol (no copy!)
 */
sSymbol *ksym_getSymbolAt(u32 address);

#endif /* KSYMBOLS_H_ */
