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

#include <esc/common.h>

#include "../../CPUInfo.h"

uint64_t MMIXCPUInfo::getRN() const {
	uint64_t rn;
	asm volatile ("GET %0,rN" : "=r"(rn));
	return rn;
}

void MMIXCPUInfo::print(FILE *f,info::cpu &cpu) {
	uint64_t rn = getRN();
	fprintf(f,"%-12s%Lu Hz\n","Speed:",cpu.speed());
	fprintf(f,"%-12s%s\n","Vendor:","THM");
	fprintf(f,"%-12s%s\n","Model:","GIMMIX");
	fprintf(f,"%-12s%Lu.%Lu.%Lu\n","Version:",rn >> 56,(rn >> 48) & 0xFF,(rn >> 40) & 0xFF);
	fprintf(f,"%-12s%Lu\n","Builddate",rn & 0xFFFFFFFFFF);
}
