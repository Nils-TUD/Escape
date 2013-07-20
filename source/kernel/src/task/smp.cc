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
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/mem/sllnodes.h>
#include <sys/task/smp.h>
#include <sys/task/proc.h>
#include <sys/log.h>
#include <sys/video.h>
#include <sys/config.h>
#include <sys/util.h>
#include <sys/cpu.h>
#include <esc/sllist.h>
#include <assert.h>
#include <string.h>

bool SMPBase::enabled;
sSLList SMPBase::cpuList;
SMP::CPU **SMPBase::cpus;
size_t SMPBase::cpuCount;

void SMPBase::init() {
	sll_init(&cpuList,slln_allocNode,slln_freeNode);

	cpuCount = 0;
	enabled = Config::get(Config::SMP) ? SMP::init_arch() : false;
	if(!enabled) {
		addCPU(true,0,true);
		setId(0,0);
	}

	log_printf("%zu CPUs found",cpuCount);
}

void SMPBase::disable() {
	cpuCount = 1;
	while(sll_length(&cpuList) > 1) {
		void *res = sll_removeIndex(&cpuList,1);
		cache_free(res);
	}
}

void SMPBase::addCPU(bool bootstrap,uint8_t id,uint8_t ready) {
	CPU *cpu = (CPU*)cache_alloc(sizeof(CPU));
	if(!cpu)
		util_panic("Unable to allocate mem for CPU");
	cpu->bootstrap = bootstrap;
	cpu->id = id;
	cpu->ready = ready;
	cpu->schedCount = 0;
	cpu->runtime = 0;
	cpu->thread = NULL;
	cpu->curCycles = 0;
	cpu->lastCycles = 0;
	cpu->lastTotal = 0;
	cpu->lastUpdate = 0;
	if(!sll_append(&cpuList,cpu))
		util_panic("Unable to append CPU");
	cpuCount++;
}

void SMPBase::setId(cpuid_t old,cpuid_t newid) {
	CPU *cpu;
	assert(newid < cpuCount);
	cpu = getCPUById(old);
	if(cpu)
		cpu->id = newid;
	if(!cpus) {
		cpus = (CPU**)cache_calloc(cpuCount,sizeof(CPU*));
		if(!cpus)
			util_panic("Not enough mem for cpu-array");
	}
	cpus[newid] = cpu;
}

void SMPBase::schedule(cpuid_t id,Thread *n,uint64_t timestamp) {
	CPU *c = cpus[id];
	if(c->thread && !(c->thread->getFlags() & T_IDLE)) {
		c->curCycles += Thread::getTSC() - c->thread->getStats().cycleStart;
		c->runtime += timestamp - c->lastSched;
	}
	if(!(n->getFlags() & T_IDLE)) {
		c->schedCount++;
		c->lastSched = timestamp;
	}
	c->thread = n;
}

void SMPBase::updateRuntimes() {
	size_t i;
	for(i = 0; i < cpuCount; i++) {
		cpus[i]->lastTotal = Thread::getTSC() - cpus[i]->lastUpdate;
		cpus[i]->lastUpdate = Thread::getTSC();
		if(cpus[i]->thread && !(cpus[i]->thread->getFlags() & T_IDLE)) {
			/* we want to measure the last second only */
			uint64_t cycles = Thread::getTSC() - cpus[i]->thread->getStats().cycleStart;
			cpus[i]->curCycles = MIN(cpus[i]->lastTotal,cpus[i]->curCycles + cycles);
		}
		cpus[i]->lastCycles = cpus[i]->curCycles;
		cpus[i]->curCycles = 0;
	}
}

void SMPBase::killThread(Thread *t) {
	if(cpuCount > 1) {
		size_t i;
		cpuid_t cur = getCurId();
		for(i = 0; i < cpuCount; i++) {
			if(i != cur && cpus[i]->ready && cpus[i]->thread == t) {
				sendIPI(i,IPI_TERM);
				break;
			}
		}
	}
}

void SMPBase::wakeupCPU() {
	if(cpuCount > 1) {
		size_t i;
		cpuid_t cur = getCurId();
		for(i = 0; i < cpuCount; i++) {
			if(i != cur && cpus[i]->ready && (!cpus[i]->thread ||
					(cpus[i]->thread->getFlags() & T_IDLE))) {
				sendIPI(i,IPI_WORK);
				break;
			}
		}
	}
}

void SMPBase::flushTLB(pagedir_t *pdir) {
	if(!cpus || cpuCount == 1)
		return;

	size_t i;
	cpuid_t cur = getCurId();
	for(i = 0; i < cpuCount; i++) {
		CPU *cpu = cpus[i];
		if(cpu && cpu->ready && i != cur) {
			Thread *t = cpu->thread;
			if(t && t->getProc()->getPageDir() == pdir)
				sendIPI(i,IPI_FLUSH_TLB);
		}
	}
}

void SMPBase::print() {
	sSLNode *n;
	vid_printf("CPUs:\n");
	for(n = sll_begin(&cpuList); n != NULL; n = n->next) {
		CPU *cpu = (CPU*)n->data;
		Thread *t = cpu->thread;
		vid_printf("\t%3s:%2x, running %d(%d:%s), schedCount=%zu, runtime=%Lu\n"
				   "\t        lastUpdate=%Lu, lastTotal=%Lu, lastCycles=%Lu\n",
				cpu->bootstrap ? "BSP" : "AP",cpu->id,t->getTid(),t->getProc()->getPid(),t->getProc()->getCommand(),
				cpu->schedCount,cpu->runtime,cpu->lastUpdate,cpu->lastTotal,cpu->lastCycles);
	}
}

SMP::CPU *SMPBase::getCPUById(cpuid_t id) {
	sSLNode *n;
	for(n = sll_begin(&cpuList); n != NULL; n = n->next) {
		CPU *cpu = (CPU*)n->data;
		if(cpu->id == id)
			return cpu;
	}
	return NULL;
}
