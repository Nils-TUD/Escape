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

#ifndef LOOKUP_H_
#define LOOKUP_H_

#include <esc/common.h>
#include "elf.h"
#include "loader.h"

/**
 * The assembler-routine that is called when a symbol hasn't been resolved yet. It calls
 * lookup_resolve and jumps to the address returned.
 */
extern void lookup_resolveStart(void);

/**
 * Will be called by lookup_resolveStart() to resolve a symbol from given library with given offset.
 * It will determine the name of the symbol by the offset and search the symbol in all known
 * libraries. Afterwards the address is stored in the corresponding position in GOT, so that
 * the next call calls the function directly.
 *
 * @param lib the library that wants to resolve the symbol
 * @param offset the offset (in bytes) in the DT_JMPREL-table.
 * @return the address of the symbol
 */
u32 lookup_resolve(sSharedLib *lib,u32 offset);

/**
 * Resolves a symbol by name
 *
 * @param skip the library to skip (don't search for symbols in it)
 * @param name the name of the symbol
 * @param value will be set to the address (absolute = already adjusted by the loadAddr)
 * @param lib will be set to the library in which the symbol has been found
 * @return the symbol if successfull or NULL
 */
Elf32_Sym *lookup_byName(sSharedLib *skip,const char *name,u32 *value,sSharedLib **lib);

/**
 * Resolves a symbol by name in the given library
 *
 * @param lib the library to look in
 * @param name the name of the symbol
 * @param value will be set to the address (absolute = already adjusted by the loadAddr)
 * @return the symbol if successfull or NULL
 */
Elf32_Sym *lookup_byNameIn(sSharedLib *lib,const char *name,u32 *value);

#endif /* LOOKUP_H_ */
