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
#include <esc/elf.h>

class OpenFile;

class ELF {
	static const int TYPE_PROG		= 0;
	static const int TYPE_INTERP	= 1;

	ELF() = delete;

public:
	struct StartupInfo {
		/* entry-point of the program */
		uintptr_t progEntry;
		/* entry-point of the dynamic-linker; will be the same as progEntry if no dl is used */
		uintptr_t linkerEntry;
		/* the beginning of the stack; only needed for MMIX */
		uintptr_t stackBegin;
	};

	/**
	 * Loads the program at given path from fs into the user-space
	 *
	 * @param path the path to the program
	 * @param info various information about the loaded program
	 * @return 0 on success
	 */
	static int load(const char *path,StartupInfo *info) {
		return doLoad(path,TYPE_PROG,info);
	}

	/**
	 * Architecture-specific finish-actions when loading from file. DO NOT call it directly!
	 *
	 * @param file the file
	 * @param eheader the elf-header
	 * @param info the startup-info
	 * @return 0 on success
	 */
	static int finish(OpenFile *file,const sElfEHeader *eheader,StartupInfo *info);

private:
	static int doLoad(const char *path,int type,StartupInfo *info);
	static int addSegment(OpenFile *file,const sElfPHeader *pheader,size_t loadSegNo,int type,int mflags);
};

#if defined(__x86__)
#	include <sys/arch/x86/task/elf.h>
#elif defined __eco32__
#	include <sys/arch/eco32/task/elf.h>
#else
#	include <sys/arch/mmix/task/elf.h>
#endif
