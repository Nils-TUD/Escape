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
#include <sys/task/smp.h>
#include <sys/video.h>
#include <sys/util.h>
#include <esc/sllist.h>
#include <string.h>

static bool enabled;
static sSLList *cpus;

void smp_init(void) {
	sSLNode *n;
	cpus = sll_create();
	if(!cpus)
		util_panic("Unable to create linked list for CPUs");

	enabled = smp_init_arch();
	if(!enabled)
		smp_addCPU(true,0);

	vid_printf("\nFound CPUs:\n");
	for(n = sll_begin(cpus); n != NULL; n = n->next) {
		sCPU *cpu = (sCPU*)n->data;
		vid_printf("\t%s %x\n",cpu->bootstrap ? "BSP" : "AP",cpu->id);
	}
}

bool smp_isEnabled(void) {
	return enabled;
}

void smp_addCPU(bool bootstrap,uint8_t id) {
	sCPU *cpu = (sCPU*)cache_alloc(sizeof(sCPU));
	if(!cpu)
		util_panic("Unable to allocate mem for CPU");
	cpu->bootstrap = bootstrap;
	cpu->id = id;
	if(!sll_append(cpus,cpu))
		util_panic("Unable to append CPU");
}

cpuid_t smp_getBSPId(void) {
	sSLNode *n;
	for(n = sll_begin(cpus); n != NULL; n = n->next) {
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
	for(n = sll_begin(cpus); n != NULL; n = n->next) {
		sCPU *cpu = (sCPU*)n->data;
		if(cpu->id == cur)
			return cpu->bootstrap;
	}
	/* never reached */
	return false;
}

cpuid_t smp_getMaxCPUId(void) {
	sSLNode *n;
	cpuid_t id = 0;
	for(n = sll_begin(cpus); n != NULL; n = n->next) {
		sCPU *cpu = (sCPU*)n->data;
		if(cpu->id > id)
			id = cpu->id;
	}
	return id;
}

const sSLList *smp_getCPUs(void) {
	return cpus;
}
