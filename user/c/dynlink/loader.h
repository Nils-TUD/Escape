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

#ifndef LOADER_H_
#define LOADER_H_

#include <esc/common.h>
#include <esc/mem.h>
#include <esc/sllist.h>
#include "elf.h"

#define TEXT_BEGIN		0x1000

#define LD_BIND_NOW		0
#define DEBUG_LOADER	0
#if DEBUG_LOADER
#	define DBGDL(x,...)	debugf(x,## __VA_ARGS__)
#else
#	define DBGDL(x,...)
#endif

typedef struct sSharedLib sSharedLib;
struct sSharedLib {
	char *name;
	tFD fd;
	sBinDesc bin;
	u32 loadAddr;
	Elf32_Dyn *dyn;
	Elf32_Word *hashTbl;
	Elf32_Rel *jmprel;
	Elf32_Sym *symbols;
	char *strtbl;
	sSLList *deps;
	bool relocated;
	bool initialized;
};

extern sSLList *libs;

/**
 * The start-function. Gets the file-descriptor for the program to load from the kernel
 *
 * @param binFd the file-descriptor
 * @return the entryPoint to jump at
 */
u32 load_setupProg(tFD binFd);

#endif /* LOADER_H_ */
