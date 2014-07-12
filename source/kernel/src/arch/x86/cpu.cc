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

#include <common.h>
#include <mem/cache.h>
#include <task/smp.h>
#include <task/timer.h>
#include <cpu.h>
#include <ostream.h>
#include <log.h>
#include <string.h>

uint64_t CPU::cpuHz;
uint64_t CPU::busHz;

static void doPrint(OStream &os,const SMP::CPU *smpcpu);

void CPU::detect() {
	if(cpuHz == 0) {
		/* detect the speed just once */
		cpuHz = Timer::detectCPUSpeed(&busHz);
		Log::get().writef("Detected %u %Lu Mhz CPU(s) on a %Lu Mhz bus\n",
			SMP::getCPUCount(),cpuHz / 1000000,busHz / 1000000);
	}
}

bool CPU::hasFeature(FeatSource src,uint64_t feat) {
	/* don't use the cpus-array here, since it is called before CPU::detect() */
	unsigned code;
	switch(src) {
		case BASIC:
			code = CPUID_GETFEATURES;
			break;
		default:
			code = CPUID_INTELFEATURES;
			break;
	}

	uint32_t unused,ecx,edx;
	cpuid(code,&unused,&unused,&ecx,&edx);
	if(feat >> 32)
		return !!(ecx & (feat >> 32));
	return !!(edx & feat);
}

void CPUBase::print(OStream &os) {
	for(auto cpu = SMP::begin(); cpu != SMP::end(); ++cpu) {
		os.writef("CPU %d:\n",cpu->id);
		doPrint(os,&*cpu);
	}
}

static void doPrint(OStream &os,const SMP::CPU *smpcpu) {
	os.writef("\t%-12s%lu Cycles\n","Total:",smpcpu->lastTotal);
	os.writef("\t%-12s%Lu Cycles\n","Non-Idle:",smpcpu->lastCycles);
	os.writef("\t%-12s%Lu Hz\n","Speed:",CPU::getSpeed());
}
