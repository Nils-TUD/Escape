/* This file defines standard ELF types, structures, and macros.
   Copyright (C) 1995-2003,2004,2005,2006,2007,2008
	Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _TASK_ELF_H
#define	_TASK_ELF_H

#include <sys/common.h>
#include <elf.h>

typedef struct {
	/* entry-point of the program */
	uintptr_t progEntry;
	/* entry-point of the dynamic-linker; will be the same as progEntry if no dl is used */
	uintptr_t linkerEntry;
	/* the beginning of the stack; only needed for MMIX */
	uintptr_t stackBegin;
} sStartupInfo;

/**
 * Loads the program at given path from fs into the user-space
 *
 * @param path the path to the program
 * @param info various information about the loaded program
 * @return 0 on success
 */
int elf_loadFromFile(const char *path,sStartupInfo *info);

/**
 * Loads the given code into the user-space. This is just intended for loading initloader
 *
 * @param code the address of the binary
 * @param length the length of the binary
 * @param info various information about the loaded program
 * @return 0 on success
 */
int elf_loadFromMem(const void *code,size_t length,sStartupInfo *info);

/**
 * Architecture-specific finish-actions when loading from memory. DO NOT call it directly!
 *
 * @param code the address of the binary
 * @param length the length of the binary
 * @param info the startup-info
 * @return 0 on success
 */
int elf_finishFromMem(const void *code,size_t length,sStartupInfo *info);

/**
 * Architecture-specific finish-actions when loading from file. DO NOT call it directly!
 *
 * @param file the file
 * @param eheader the elf-header
 * @param info the startup-info
 * @return 0 on success
 */
int elf_finishFromFile(tFileNo file,const sElfEHeader *eheader,sStartupInfo *info);

#endif	/* TASK_ELF_H_ */
