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
#include <sys/proc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "init.h"
#include "loader.h"
#include "lookup.h"
#include "reloc.h"
#include "setup.h"

sSharedLib *libs = NULL;

extern void initHeap(void);

void load_error(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	fprintf(stderr,"[dynlink] Error while loading process %d: ",getpid());
	vfprintf(stderr,fmt,ap);
	if(errno != 0)
		fprintf(stderr,": %s",strerror(errno));
	fprintf(stderr,"\n");
	va_end(ap);
	exit(1);
}

bool load_hasDyn(sElfDyn *dyn,ElfDynTag tag) {
	if(dyn == NULL)
		load_error("No dynamic entries");
	while(dyn->d_tag != DT_NULL) {
		if(dyn->d_tag == tag)
			return true;
		dyn++;
	}
	return false;
}

ElfDynVal load_getDyn(sElfDyn *dyn,ElfDynTag tag) {
	if(dyn == NULL)
		load_error("No dynamic entries");
	while(dyn->d_tag != DT_NULL) {
		if(dyn->d_tag == tag)
			return dyn->d_un.d_val;
		dyn++;
	}
	return 0;
}

void load_initHeap(void) {
	initHeap();
#ifndef NDEBUG
	/* allocate a lot of memory at the beginning to make the beginning of shared libraries more
	 * predictable */
	free(malloc(MAX_MEM));
#endif
}

#if defined(__i586__)
uintptr_t load_setupProg(int binFd,A_UNUSED uintptr_t a,A_UNUSED uintptr_t b,A_UNUSED size_t c,
	int argc,char **argv) {
#else
uintptr_t load_setupProg(int binFd,int argc,char **argv) {
#endif
	sSharedLib *prog;
	uintptr_t entryPoint;

	/* create entry for program */
	prog = (sSharedLib*)malloc(sizeof(sSharedLib));
	if(!prog)
		load_error("Not enough mem!");
	prog->isDSO = false;
	prog->relocated = false;
	prog->initialized = false;
	prog->dynstrtbl = NULL;
	prog->dyn = NULL;
	prog->name = "-Main-";
	prog->deps = NULL;
	appendto(&libs,prog);

	/* load program including shared libraries into linked list */
	load_doLoad(binFd,prog);

	/* load segments into memory */
	entryPoint = load_addSegments();

#if PRINT_LOADADDR
	for(sSharedLib *l = libs; l != NULL; l = l->next) {
		uintptr_t addr;
		lookup_byName(NULL,"_start",&addr);
		debugf("[%d] Loaded %s @ %p .. %p (text @ %p) with deps: ",
				getpid(),l->name,l->loadAddr,l->loadAddr + l->textSize,addr);
		for(sDep *dl = l->deps; dl != NULL; dl = dl->next)
			debugf("%s ",dl->lib->name);
		debugf("\n");
	}
#endif

	/* relocate everything we need so that the program can start */
	load_reloc();

	/* call global constructors */
	load_init(argc,argv);

	return entryPoint;
}
