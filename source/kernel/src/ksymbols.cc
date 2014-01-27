/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <sys/common.h>
#include <sys/ksymbols.h>
#include <sys/video.h>

extern KSymbols::Symbol kernel_symbols[];
extern size_t kernel_symbol_count;

void KSymbols::print(OStream &os) {
	os.writef("Kernel-Symbols:\n");
	for(size_t i = 0; i < kernel_symbol_count; i++)
		os.writef("\t%p -> %s\n",kernel_symbols[i].address,kernel_symbols[i].funcName);
}

KSymbols::Symbol *KSymbols::getSymbolAt(uintptr_t address) {
	if(address < KERNEL_AREA)
		return NULL;

	Symbol *sym = &kernel_symbols[kernel_symbol_count - 1];
	while(sym >= &kernel_symbols[0]) {
		if(address >= sym->address)
			return sym;
		sym--;
	}
	return &kernel_symbols[0];
}
