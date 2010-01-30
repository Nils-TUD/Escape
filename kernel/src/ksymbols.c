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

#include <common.h>
#include <ksymbols.h>
#include <video.h>

static sSymbol ksymbols[] = {
	/* add dummy-entry to prevent empty array */
	{0,""},
#if DEBUGGING
#	if TESTING
#		include "../../build/debug/kernelt_symbols.txt"
#	else
#		include "../../build/debug/kernel_symbols.txt"
#	endif
#else
#	if TESTING
#		include "../../build/release/kernelt_symbols.txt"
#	else
#		include "../../build/release/kernel_symbols.txt"
#	endif
#endif
};

void ksym_print(void) {
	u32 i;
	vid_printf("Kernel-Symbols:\n");
	for(i = 0; i < ARRAY_SIZE(ksymbols); i++)
		vid_printf("\t0x%08x -> %s\n",ksymbols[i].address,ksymbols[i].funcName);
}

sSymbol *ksym_getSymbolAt(u32 address) {
	sSymbol *sym = &ksymbols[ARRAY_SIZE(ksymbols) - 1];
	while(sym >= &ksymbols[0]) {
		if(address >= sym->address)
			return sym;
		sym--;
	}
	return &ksymbols[0];
}
