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

#pragma once

#include <sys/common.h>
#include <sys/elf.h>
#include <sys/debug.h>
#include <sys/mman.h>

#define LD_BIND_NOW		0
#define DEBUG_LOADER	0
#define PRINT_LOADADDR	0
#if DEBUG_LOADER
#	define DBGDL(x,...)	debugf("[%d] " x,getpid(),## __VA_ARGS__)
#else
#	define DBGDL(x,...)
#endif

#define MAX_MEM			(1024 * 64)

typedef struct sSharedLib sSharedLib;

typedef struct sDep {
	sSharedLib *lib;
	struct sDep *next;
} sDep;

struct sSharedLib {
	uchar isDSO;
	const char *name;
	int fd;
	uintptr_t textAddr;
	uintptr_t loadAddr;
	size_t textSize;
	sElfDyn *dyn;
	ElfWord *hashTbl;
	uint jmprelType;
	sElfRel *jmprel;
	sElfSym *dynsyms;
	char *dynstrtbl;
	sDep *deps;
	bool relocated;
	bool initialized;
	sSharedLib *next;
};

static inline void appendto(sSharedLib **list,sSharedLib *lib) {
	sSharedLib *p = NULL,*l = *list;
	while(l) {
		p = l;
		l = l->next;
	}
	if(p)
		p->next = lib;
	else
		*list = lib;
	lib->next = NULL;
}

extern sSharedLib *libs;

/**
 * Prints the given error-message, including errno, and exits
 *
 * @param fmt the message
 */
void load_error(const char *fmt,...);

/**
 * Inits the heap
 */
void load_initHeap(void);

/**
 * The start-function. Gets the file-descriptor for the program to load from the kernel
 *
 * @param binFd the file-descriptor
 * @param argc argc for the program
 * @param argv argv for the program
 * @return the entryPoint to jump at
 */
#if defined(__i586__)
uintptr_t load_setupProg(int binFd,uintptr_t,uintptr_t,size_t,int argc,char **argv);
#else
uintptr_t load_setupProg(int binFd,int argc,char **argv);
#endif

/**
 * Searches for the given tag in the dynamic-section
 *
 * @param dyn the dynamic section
 * @param tag the tag to find
 * @return true if found
 */
bool load_hasDyn(sElfDyn *dyn,ElfDynTag tag);

/**
 * Determines the value of the given tag in the dynamic-section
 *
 * @param dyn the dynamic section
 * @param tag the tag to find
 * @return the value
 */
ElfDynVal load_getDyn(sElfDyn *dyn,ElfDynTag tag);
