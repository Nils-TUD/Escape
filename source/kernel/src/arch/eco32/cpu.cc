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
#include <sys/mem/kheap.h>
#include <sys/task/smp.h>
#include <sys/cpu.h>
#include <sys/ostream.h>
#include <sys/video.h>
#include <string.h>

uint64_t CPU::cpuHz;

void CPUBase::print(OStream &os) {
	auto cpu = SMP::begin();
	os.writef("CPU 0:\n");
	os.writef("\t%-12s%Lu Cycles\n","Total:",cpu->lastTotal);
	os.writef("\t%-12s%Lu Cycles\n","Non-Idle:",cpu->lastCycles);
	os.writef("\t%-12s%Lu Hz\n","Speed:",CPU::cpuHz);
}
