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
#include <sys/video.h>
#include <sys/util.h>
#include <esc/sllist.h>
#include <string.h>

static sCPU *smp_getCPUById(cpuid_t id);

static bool enabled;
static sSLList cpus;

void smp_init(void) {
	sll_init(&cpus,slln_allocNode,slln_freeNode);

	enabled = smp_init_arch();
	if(!enabled)
		smp_addCPU(true,0,true);

	vid_printf("%zu CPUs found",sll_length(&cpus));
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
	if(!sll_append(&cpus,cpu))
		util_panic("Unable to append CPU");
}

void smp_setReady(cpuid_t id) {
	sCPU *cpu = smp_getCPUById(id);
	if(cpu)
		cpu->ready = true;
}

void smp_flushTLB(tPageDir *pdir) {
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

void smp_wakeupCPU(void) {
	sSLNode *n;
	cpuid_t cur = smp_getCurId();
	for(n = sll_begin(&cpus); n != NULL; n = n->next) {
		sCPU *cpu = (sCPU*)n->data;
		if(cpu->ready && cpu->id != cur) {
			sThread *t = smp_getThreadOf(cpu->id);
			if(t->flags & T_IDLE) {
				smp_sendIPI(cpu->id,IPI_WORK);
				break;
			}
		}
	}
}

void smp_setId(cpuid_t old,cpuid_t new) {
	sCPU *cpu = smp_getCPUById(old);
	if(cpu)
		cpu->id = new;
}

cpuid_t smp_getBSPId(void) {
	sSLNode *n;
	for(n = sll_begin(&cpus); n != NULL; n = n->next) {
		sCPU *cpu = (sCPU*)n->data;
		if(cpu->bootstrap)
			return cpu->id;
	}
	/* never reached */
	return 0;
}

bool smp_isBSP(void) {
	cpuid_t cur = smp_getCurId();
	sSLNode *n;
	for(n = sll_begin(&cpus); n != NULL; n = n->next) {
		sCPU *cpu = (sCPU*)n->data;
		if(cpu->id == cur)
			return cpu->bootstrap;
	}
	/* never reached */
	return false;
}

size_t smp_getCPUCount(void) {
	return cpus.length;
}

const sSLList *smp_getCPUs(void) {
	return &cpus;
}

void smp_print(void) {
	sSLNode *n;
	vid_printf("CPUs:\n");
	for(n = sll_begin(&cpus); n != NULL; n = n->next) {
		sCPU *cpu = (sCPU*)n->data;
		sThread *t = smp_getThreadOf(cpu->id);
		vid_printf("\t%s:%x, running %d(%d:%s)\n",cpu->bootstrap ? "BSP" : "AP",cpu->id,
				 t->tid,t->proc->pid,t->proc->command);
	}
}

static sCPU *smp_getCPUById(cpuid_t id) {
	sSLNode *n;
	for(n = sll_begin(&cpus); n != NULL; n = n->next) {
		sCPU *cpu = (sCPU*)n->data;
		if(cpu->id == id)
			return cpu;
	}
	return NULL;
}
