/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <sys/machine/fpu.h>
#include <sys/machine/cpu.h>
#include <sys/mem/kheap.h>
#include <sys/video.h>
#include <string.h>

/**
 * finit instruction
 */
extern void fpu_finit(void);

/**
 * Saves the state to the given sFPUState-structure
 *
 * @param state the state
 */
extern void fpu_saveState(sFPUState *state);

/**
 * Restore the state to the given sFPUState-structure
 *
 * @param state the state to write to
 */
extern void fpu_restoreState(sFPUState *state);

/* current FPU state-memory */
static sFPUState **curFPUState = NULL;

void fpu_init(void) {
	u32 cr0 = cpu_getCR0();
	/* enable coprocessor monitoring */
	cr0 |= CR0_MONITOR_COPROC;
	/* disable emulate */
	cr0 &= ~CR0_EMULATE;
	cpu_setCR0(cr0);

	/* TODO check whether we have a FPU */
	/* set the OSFXSR bit
	cpu_setCR4(cpu_getCR4() | 0x200);*/
	/* init the fpu */
	fpu_finit();
}

void fpu_lockFPU(void) {
	/* set the task-switched-bit in CR0. as soon as a process uses any FPU instruction
	 * we'll get a EX_CO_PROC_NA and call fpu_handle() */
	cpu_setCR0(cpu_getCR0() | CR0_TASK_SWITCHED);
}

void fpu_handleCoProcNA(sFPUState **state) {
	if(curFPUState != state) {
		/* if any process has used the FPU in the past */
		if(curFPUState != NULL) {
			/* do we have to allocate space for the state? */
			if(*curFPUState == NULL) {
				*curFPUState = (sFPUState*)kheap_alloc(sizeof(sFPUState));
				/* if we can't save the state, don't unlock the FPU for another process */
				/* TODO ok? */
				if(*curFPUState == NULL)
					return;
			}
			/* unlock FPU (necessary here because otherwise fpu_saveState would cause a #NM) */
			cpu_setCR0(cpu_getCR0() & ~CR0_TASK_SWITCHED);
			/* save state */
			fpu_saveState(*curFPUState);
		}
		else
			cpu_setCR0(cpu_getCR0() & ~CR0_TASK_SWITCHED);

		/* init FPU for new process */
		curFPUState = state;
		if(*state != NULL)
			fpu_restoreState(*state);
		else
			fpu_finit();
	}
	/* just unlock */
	else
		cpu_setCR0(cpu_getCR0() & ~CR0_TASK_SWITCHED);
}

void fpu_cloneState(sFPUState **dst,const sFPUState *src) {
	if(src != NULL) {
		*dst = (sFPUState*)kheap_alloc(sizeof(sFPUState));
		/* simply ignore it here if alloc fails */
		if(*dst)
			memcpy(*dst,src,sizeof(sFPUState));
	}
	else
		*dst = NULL;
}

void fpu_freeState(sFPUState **state) {
	if(*state != NULL)
		kheap_free(*state);
	*state = NULL;
	/* we have to unset the current state because maybe the next created process gets
	 * the same slot, so that the pointer is the same. */
	if(state == curFPUState)
		curFPUState = NULL;
}
