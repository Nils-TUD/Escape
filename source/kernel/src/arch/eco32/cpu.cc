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
#include <sys/ostream.h>
#include <sys/video.h>
#include <string.h>

void CPUBase::print(OStream &os) {
	const SMP::CPU *smpcpu = SMP::getCPUs()[0];
	os.writef("CPU 0:\n");
	os.writef("\t%-12s%Lu Cycles\n","Total:",smpcpu->lastTotal);
	os.writef("\t%-12s%Lu Cycles\n","Non-Idle:",smpcpu->lastCycles);
	os.writef("\t%-12s%lu Hz\n","Speed:",0);
	os.writef("\t%-12s%s\n","Vendor:","THM");
	os.writef("\t%-12s%s\n","Model:","ECO32");
}
