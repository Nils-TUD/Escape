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

#include <esc/common.h>
#include <esc/proc.h>
#include <stdlib.h>
#include <stdio.h>
#include "loader.h"
#include "init.h"
#include "reloc.h"
#include "setup.h"

sSLList *libs = NULL;

void load_error(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	printe("[%d] Error: ",getpid());
	vprinte(fmt,ap);
	if(errno < 0)
		printe(": %s",strerror(errno));
	printe("\n");
	va_end(ap);
	exit(1);
}

uint32_t load_getDyn(Elf32_Dyn *dyn,Elf32_Sword tag) {
	if(dyn == NULL)
		load_error("No dynamic entries");
	while(dyn->d_tag != DT_NULL) {
		if(dyn->d_tag == tag)
			return dyn->d_un.d_val;
		dyn++;
	}
	return 0;
}

uintptr_t load_setupProg(int binFd) {
	uintptr_t entryPoint;
	libs = sll_create();
	if(!libs)
		load_error("Not enough mem!");

	/* create entry for program */
	sSharedLib *prog = (sSharedLib*)malloc(sizeof(sSharedLib));
	if(!prog)
		load_error("Not enough mem!");
	prog->isDSO = false;
	prog->relocated = false;
	prog->initialized = false;
	prog->dynstrtbl = NULL;
	prog->dyn = NULL;
	prog->name = "-Main-";
	prog->deps = sll_create();
	if(!prog->deps || !sll_append(libs,prog))
		load_error("Not enough mem!");

	/* load program including shared libraries into linked list */
	load_doLoad(binFd,prog);

	/* load segments into memory */
	entryPoint = load_addSegments();

#if PRINT_LOADADDR
	sSLNode *n,*m;
	for(n = sll_begin(libs); n != NULL; n = n->next) {
		sSharedLib *l = (sSharedLib*)n->data;
		debugf("[%d] Loaded %s @ %p .. %p with deps: ",
				getpid(),l->name,l->loadAddr,l->loadAddr + l->textSize);
		for(m = sll_begin(l->deps); m != NULL; m = m->next) {
			sSharedLib *dl = (sSharedLib*)m->data;
			debugf("%s ",dl->name);
		}
		debugf("\n");
	}
#endif

	/* relocate everything we need so that the program can start */
	load_reloc();

	/* call global constructors */
	load_init();

	return entryPoint;
}
