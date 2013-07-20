/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <sys/mem/kheap.h>
#include <sys/task/smp.h>
#include <sys/cpu.h>
#include <sys/printf.h>
#include <sys/video.h>
#include <string.h>

void CPUBase::sprintf(sStringBuffer *buf) {
	const SMP::CPU *smpcpu = SMP::getCPUs()[0];
	prf_sprintf(buf,"CPU 0:\n");
	prf_sprintf(buf,"\t%-12s%Lu Cycles\n","Total:",smpcpu->lastTotal);
	prf_sprintf(buf,"\t%-12s%Lu Cycles\n","Non-Idle:",smpcpu->lastCycles);
	prf_sprintf(buf,"\t%-12s%lu Hz\n","Speed:",0);
	prf_sprintf(buf,"\t%-12s%s\n","Vendor:","THM");
	prf_sprintf(buf,"\t%-12s%s\n","Model:","ECO32");
}
