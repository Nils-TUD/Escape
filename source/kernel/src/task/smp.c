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
#include <sys/log.h>
#include <sys/video.h>
#include <sys/util.h>
#include <esc/sllist.h>
#include <assert.h>
#include <string.h>

static sCPU *smp_getCPUById(cpuid_t id);

static bool enabled;
static sSLList cpuList;
static sCPU **cpus;
static size_t cpuCount;

void smp_init(void) {
	sll_init(&cpuList,slln_allocNode,slln_freeNode);

	cpuCount = 0;
	enabled = smp_init_arch();
	if(!enabled) {
		smp_addCPU(true,0,true);
		smp_setId(0,0);
	}

	log_printf("%zu CPUs found",cpuCount);
}

bool smp_isEnabled(void) {
	return enabled;
}

void smp_addCPU(bool bootstrap,uint8_t id,uint8_t ready) {
	sCPU *cpu = (sCPU*)cache_alloc(sizeof(sCPU));
	if(!cpu)
		util_panic("Unable to allocate mem for CPU");
	cpu->bootstrap = bootstrap;
	cpu->id = id;
	cpu->ready = ready;
	cpu->schedCount = 0;
	cpu->runtime = 0;
	cpu->thread = NULL;
	if(!sll_append(&cpuList,cpu))
		util_panic("Unable to append CPU");
	cpuCount++;
}

void smp_setId(cpuid_t old,cpuid_t new) {
	sCPU *cpu;
	assert(new < cpuCount);
	cpu = smp_getCPUById(old);
	if(cpu)
		cpu->id = new;
	if(!cpus) {
		cpus = (sCPU**)cache_alloc(cpuCount * sizeof(sCPU*));
		if(!cpus)
			util_panic("Not enough mem for cpu-array");
	}
	cpus[new] = cpu;
}

void smp_setReady(cpuid_t id) {
	assert(cpus[id]);
	cpus[id]->ready = true;
}

void smp_schedule(cpuid_t id,sThread *new,uint64_t timestamp) {
	sCPU *c = cpus[id];
	if(c->thread && !(c->thread->flags & T_IDLE))
		c->runtime += timestamp - c->lastSched;
	if(!(new->flags & T_IDLE)) {
		c->schedCount++;
		c->lastSched = timestamp;
	}
	c->thread = new;
}

void smp_killThread(sThread *t) {
	if(cpuCount > 1) {
		size_t i;
		cpuid_t cur = smp_getCurId();
		for(i = 0; i < cpuCount; i++) {
			if(i != cur && cpus[i]->ready && cpus[i]->thread == t) {
				smp_sendIPI(i,IPI_TERM);
				break;
			}
		}
	}
}

void smp_wakeupCPU(void) {
	if(cpuCount > 1) {
		size_t i;
		cpuid_t cur = smp_getCurId();
		for(i = 0; i < cpuCount; i++) {
			if(i != cur && cpus[i]->ready && (!cpus[i]->thread || (cpus[i]->thread->flags & T_IDLE))) {
				smp_sendIPI(i,IPI_WORK);
				break;
			}
		}
	}
}

void smp_flushTLB(pagedir_t *pdir) {
#if 0
	sSLNode *n;
	cpuid_t cur = smp_getCurId();
	for(n = sll_begin(&cpus); n != NULL; n = n->next) {
		sCPU *cpu = (sCPU*)n->data;
		if(cpu->ready && cpu->id != cur) {
			sThread *t = smp_getThreadOf(cpu->id);
			if((uintptr_t)t == 0xc084fdbc - 0x18)
				util_panic("hier");
			if(&t->proc->pagedir == pdir)
				smp_sendIPI(cpu->id,IPI_FLUSH_TLB);
		}
	}
#endif
}

bool smp_isBSP(void) {
	cpuid_t cur = smp_getCurId();
	return cpus[cur]->bootstrap;
}

size_t smp_getCPUCount(void) {
	return cpuCount;
}

sCPU **smp_getCPUs(void) {
	return cpus;
}

void smp_print(void) {
	sSLNode *n;
	vid_printf("CPUs:\n");
	for(n = sll_begin(&cpuList); n != NULL; n = n->next) {
		sCPU *cpu = (sCPU*)n->data;
		sThread *t = cpu->thread;
		vid_printf("\t%s:%x, running %d(%d:%s), schedCount=%zu, runtime=%Lu\n",
				cpu->bootstrap ? "BSP" : "AP",cpu->id,t->tid,t->proc->pid,t->proc->command,
				cpu->schedCount,cpu->runtime);
	}
}

static sCPU *smp_getCPUById(cpuid_t id) {
	sSLNode *n;
	for(n = sll_begin(&cpuList); n != NULL; n = n->next) {
		sCPU *cpu = (sCPU*)n->data;
		if(cpu->id == id)
			return cpu;
	}
	return NULL;
}
