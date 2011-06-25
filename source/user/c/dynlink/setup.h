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

#ifndef LINKER_H_
#define LINKER_H_

#include <esc/common.h>
#include <sys/task/elf.h>
#include <esc/debug.h>
#include <esc/sllist.h>
#include <esc/mem.h>

/*#define CALLTRACE_PID	10*/

#define LD_BIND_NOW		0
#define DEBUG_LOADER	0
#define PRINT_LOADADDR	0
#if DEBUG_LOADER
#	define DBGDL(x,...)	debugf("[%d] " x,getpid(),## __VA_ARGS__)
#else
#	define DBGDL(x,...)
#endif

#define TEXT_BEGIN		0x1000

typedef struct sSharedLib sSharedLib;
struct sSharedLib {
	uchar isDSO;
	const char *name;
	int fd;
	sBinDesc bin;
	uintptr_t loadAddr;
	size_t textSize;
	Elf32_Dyn *dyn;
	Elf32_Word *hashTbl;
	Elf32_Rel *jmprel;
	Elf32_Sym *dynsyms;
	char *dynstrtbl;
	sSLList *deps;
	bool relocated;
	bool initialized;
};

extern sSLList *libs;

/**
 * Prints the given error-message, including errno, and exits
 *
 * @param fmt the message
 */
void load_error(const char *fmt,...);

/**
 * The start-function. Gets the file-descriptor for the program to load from the kernel
 *
 * @param binFd the file-descriptor
 * @return the entryPoint to jump at
 */
uintptr_t load_setupProg(int binFd);

/**
 * Determines the value of the given tag in the dynamic-section
 *
 * @param dyn the dynamic section
 * @param tag the tag to find
 * @return the value
 */
uint load_getDyn(Elf32_Dyn *dyn,Elf32_Sword tag);

#endif /* LINKER_H_ */
