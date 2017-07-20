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
#include <sys/elf.h>
#include <sys/proc.h>

#include "init.h"
#include "lookup.h"
#include "setup.h"

typedef uintptr_t (*fPreinit)(uintptr_t,int,char *[]);
typedef void (*init_func)(void);

static void load_initLib(sSharedLib *l);

void load_init(int argc,char **argv) {
	uintptr_t addr;
	if(lookup_byName(NULL,"__libc_preinit",&addr)) {
		fPreinit preinit = (fPreinit)addr;
		preinit(0,argc,argv);
	}

	for(sSharedLib *l = libs; l != NULL; l = l->next)
		load_initLib(l);
}

static void load_initLib(sSharedLib *l) {
	/* already initialized? */
	if(l->initialized)
		return;

	/* first go through the dependencies */
	for(sDep *dl = l->deps; dl != NULL; dl = dl->next)
		load_initLib(dl->lib);

	/* if its not the executable, call the init-function */
	if(l->isDSO) {
		uintptr_t initAddr = (uintptr_t)load_getDyn(l->dyn,DT_INIT);
		if(initAddr) {
			DBGDL("Calling _init of %s...\n",l->name);
			void (*initFunc)(void) = (void (*)(void))(initAddr + l->loadAddr);
			initFunc();
		}

		uintptr_t initArrayAddr = (uintptr_t)load_getDyn(l->dyn,DT_INIT_ARRAY);
		if(initArrayAddr) {
			size_t initArraySize = (size_t)load_getDyn(l->dyn,DT_INIT_ARRAYSZ) / sizeof(init_func);
			init_func *funcs = (init_func*)(initArrayAddr + l->loadAddr);
			for(; initArraySize-- > 0; ++funcs) {
				DBGDL("Calling init_array function %p of %s...\n",*funcs,l->name);
				(*funcs)();
			}
		}
	}

	l->initialized = true;
}
