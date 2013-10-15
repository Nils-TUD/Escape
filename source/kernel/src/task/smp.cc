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
#include <sys/mem/pagedir.h>
#include <sys/mem/cache.h>
#include <sys/task/smp.h>
#include <sys/task/proc.h>
#include <sys/log.h>
#include <sys/config.h>
#include <sys/util.h>
#include <sys/cpu.h>
#include <assert.h>
#include <string.h>

bool SMPBase::enabled;
SList<SMP::CPU> SMPBase::cpuList;
SMP::CPU **SMPBase::cpus;
size_t SMPBase::cpuCount;

void SMPBase::init() {
	cpuCount = 0;
	enabled = Config::get(Config::SMP) ? SMP::initArch() : false;
	if(!enabled) {
		addCPU(true,0,true);
		setId(0,0);
	}

	Log::get().writef("%zu CPUs found",cpuCount);
}

void SMPBase::disable() {
	cpuCount = 1;
	CPU *prev = &*cpuList.begin();
	while(cpuList.length() > 1) {
		CPU *cpu = &*(++cpuList.begin());
		cpuList.removeAt(prev,cpu);
		delete cpu;
	}
}

void SMPBase::addCPU(bool bootstrap,uint8_t id,uint8_t ready) {
	CPU *cpu = new CPU(id,bootstrap,ready);
	if(!cpu)
		Util::panic("Unable to allocate mem for CPU");
	cpuList.append(cpu);
	cpuCount++;
}

void SMPBase::setId(cpuid_t old,cpuid_t newid) {
	assert(newid < cpuCount);
	CPU *cpu = getCPUById(old);
	if(cpu)
		cpu->id = newid;
	if(!cpus) {
		cpus = (CPU**)Cache::calloc(cpuCount,sizeof(CPU*));
		if(!cpus)
			Util::panic("Not enough mem for cpu-array");
	}
	cpus[newid] = cpu;
}

void SMPBase::updateRuntimes() {
	uint64_t now = Thread::getTSC();
	for(auto cpu = cpuList.begin(); cpu != cpuList.end(); ++cpu) {
		cpu->lastTotal = now - cpu->lastUpdate;
		cpu->lastUpdate = now;
		if(cpu->thread && !(cpu->thread->getFlags() & T_IDLE)) {
			/* we want to measure the last second only */
			uint64_t cycles = now - cpu->thread->getStats().cycleStart;
			cpu->curCycles = MIN(cpu->lastTotal,cpu->curCycles + cycles);
		}
		cpu->lastCycles = cpu->curCycles;
		cpu->curCycles = 0;
	}
}

void SMPBase::killThread(Thread *t) {
	if(cpuCount > 1) {
		cpuid_t cur = getCurId();
		for(auto cpu = cpuList.cbegin(); cpu != cpuList.cend(); ++cpu) {
			if(cpu->id != cur && cpu->ready && cpu->thread == t) {
				sendIPI(cpu->id,IPI_TERM);
				break;
			}
		}
	}
}

void SMPBase::wakeupCPU() {
	if(cpuCount > 1) {
		cpuid_t cur = getCurId();
		for(auto cpu = cpuList.cbegin(); cpu != cpuList.cend(); ++cpu) {
			if(cpu->id != cur && cpu->ready && (!cpu->thread ||
					(cpu->thread->getFlags() & T_IDLE))) {
				sendIPI(cpu->id,IPI_WORK);
				break;
			}
		}
	}
}

void SMPBase::flushTLB(PageDir *pdir) {
	if(!cpus || cpuCount == 1)
		return;

	cpuid_t cur = getCurId();
	for(auto cpu = cpuList.cbegin(); cpu != cpuList.cend(); ++cpu) {
		if(cpu->ready && cpu->id != cur) {
			Thread *t = cpu->thread;
			if(t && t->getProc()->getPageDir() == pdir)
				sendIPI(cpu->id,IPI_FLUSH_TLB);
		}
	}
}

void SMPBase::callback(cpuid_t id) {
	CPU *c = cpus[id];
	assert(c->callback);
	c->callback();
}

void SMPBase::callbackOthers(callback_func callback) {
	if(!cpus || cpuCount == 1)
		return;

	cpuid_t cur = getCurId();
	for(auto cpu = cpuList.begin(); cpu != cpuList.end(); ++cpu) {
		if(cpu->ready && cpu->id != cur) {
			cpu->callback = callback;
			sendIPI(cpu->id,IPI_CALLBACK);
		}
	}
}

void SMPBase::print(OStream &os) {
	os.writef("CPUs:\n");
	for(auto cpu = cpuList.cbegin(); cpu != cpuList.cend(); ++cpu) {
		Thread *t = cpu->thread;
		os.writef("\t%3s:%2x, running %d(%d:%s), lastUpdate=%Lu, lastTotal=%Lu, lastCycles=%Lu\n",
				cpu->bootstrap ? "BSP" : "AP",cpu->id,t->getTid(),t->getProc()->getPid(),
				t->getProc()->getProgram(),cpu->lastUpdate,cpu->lastTotal,cpu->lastCycles);
	}
}

SMP::CPU *SMPBase::getCPUById(cpuid_t id) {
	for(auto cpu = cpuList.begin(); cpu != cpuList.end(); ++cpu) {
		if(cpu->id == id)
			return &*cpu;
	}
	return NULL;
}
