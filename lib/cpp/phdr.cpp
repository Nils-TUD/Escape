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

#include <sys/task/elf.h>
#include <stddef.h>
#include <stdlib.h>

/*#define GL(x) _##x*/

typedef int (*fCallback)(struct dl_phdr_info * info,size_t size,void *data);

/* make gcc happy */
extern "C" int dl_iterate_phdr(fCallback callback,void *data);

extern u32 progHeader;
extern u32 progHeaderSize;

extern "C" int dl_iterate_phdr(fCallback callback,void *data) {
	u32 progHeaderNum = progHeaderSize / sizeof(Elf32_Phdr);
	if(progHeaderNum == 0)
		error("Trying to unwind stack but have no program-headers (in statically linked prog)!?");

    struct dl_phdr_info info;
	info.dlpi_addr = 0;
	info.dlpi_name = "";
	info.dlpi_phdr = (const Elf32_Phdr*)progHeader;
	info.dlpi_phnum = progHeaderNum;
	/* in statically linked progs, atm we have no way to add shared objects */
	info.dlpi_adds = 0;
	info.dlpi_subs = 0;
	if(!(*callback)(&info,sizeof(struct dl_phdr_info),data))
		error("Unable to find exception-header!?");
	return 1;
}
