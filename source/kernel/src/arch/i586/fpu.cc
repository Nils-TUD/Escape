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
#include <sys/arch/i586/fpu.h>
#include <sys/task/smp.h>
#include <sys/task/thread.h>
#include <sys/mem/cache.h>
#include <sys/video.h>
#include <sys/cpu.h>
#include <sys/util.h>
#include <string.h>

extern void fpu_finit(void);
extern void fpu_saveState(sFPUState *state);
extern void fpu_restoreState(sFPUState *state);

/* current FPU state-memory */
static sFPUState ***curStates = NULL;

void fpu_preinit(void) {
	uint32_t cr0 = CPU::getCR0();
	/* enable coprocessor monitoring */
	cr0 |= CPU::CR0_MONITOR_COPROC;
	/* disable emulate */
	cr0 &= ~CPU::CR0_EMULATE;
	CPU::setCR0(cr0);

	/* TODO check whether we have a FPU */
	/* set the OSFXSR bit
	CPU::setCR4(CPU::getCR4() | 0x200);*/
	/* init the fpu */
	__asm__ volatile (
		"finit"
	);
}

void fpu_init(void) {
	/* allocate a state-pointer for each cpu */
	curStates = (sFPUState***)cache_calloc(SMP::getCPUCount(),sizeof(sFPUState**));
	if(!curStates)
		util_panic("Unable to allocate memory for FPU-states");
}

void fpu_lockFPU(void) {
	/* set the task-switched-bit in CR0. as soon as a process uses any FPU instruction
	 * we'll get a EX_CO_PROC_NA and call fpu_handleCoProcNA() */
	CPU::setCR0(CPU::getCR0() | CPU::CR0_TASK_SWITCHED);
}

void fpu_handleCoProcNA(sFPUState **state) {
	Thread *t = Thread::getRunning();
	sFPUState **current = curStates[t->getCPU()];
	if(current != state) {
		/* if any process has used the FPU in the past */
		if(current != NULL) {
			/* do we have to allocate space for the state? */
			if(*current == NULL) {
				*current = (sFPUState*)cache_alloc(sizeof(sFPUState));
				/* if we can't save the state, don't unlock the FPU for another process */
				/* TODO ok? */
				if(*current == NULL)
					return;
			}
			/* unlock FPU (necessary here because otherwise fpu_saveState would cause a #NM) */
			CPU::setCR0(CPU::getCR0() & ~CPU::CR0_TASK_SWITCHED);
			/* save state */
			__asm__ volatile (
				"fsave %0" : : "m" (**current)
			);
		}
		else
			CPU::setCR0(CPU::getCR0() & ~CPU::CR0_TASK_SWITCHED);

		/* init FPU for new process */
		curStates[t->getCPU()] = state;
		if(*state != NULL) {
			__asm__ volatile (
				"frstor %0" : : "m" (**state)
			);
		}
		else {
			__asm__ volatile (
				"finit"
			);
		}
	}
	/* just unlock */
	else
		CPU::setCR0(CPU::getCR0() & ~CPU::CR0_TASK_SWITCHED);
}

void fpu_cloneState(sFPUState **dst,const sFPUState *src) {
	if(src != NULL) {
		*dst = (sFPUState*)cache_alloc(sizeof(sFPUState));
		/* simply ignore it here if alloc fails */
		if(*dst)
			memcpy(*dst,src,sizeof(sFPUState));
	}
	else
		*dst = NULL;
}

void fpu_freeState(sFPUState **state) {
	size_t i,n;
	if(*state != NULL)
		cache_free(*state);
	*state = NULL;
	/* we have to unset the current state because maybe the next created process gets
	 * the same slot, so that the pointer is the same. */
	for(i = 0, n = SMP::getCPUCount(); i < n; i++) {
		if(curStates[i] == state)
			curStates[i] = NULL;
	}
}
