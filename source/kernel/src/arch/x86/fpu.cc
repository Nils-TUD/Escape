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
#include <sys/arch/x86/fpu.h>
#include <sys/task/smp.h>
#include <sys/task/thread.h>
#include <sys/mem/cache.h>
#include <sys/video.h>
#include <sys/cpu.h>
#include <sys/util.h>
#include <string.h>

/* current FPU state-memory */
FPU::XState ***FPU::curStates = NULL;

void FPU::init() {
	/* TODO check whether we have a FPU/SSE/... */

	ulong cr4 = CPU::getCR4();
	cr4 |= CPU::CR4_OSFXSR;
	cr4 |= CPU::CR4_OSXMMEXCPT;
	CPU::setCR4(cr4);

	ulong cr0 = CPU::getCR0();
	/* enable coprocessor monitoring */
	cr0 |= CPU::CR0_MONITOR_COPROC;
	/* disable emulate */
	cr0 &= ~CPU::CR0_EMULATE;
	CPU::setCR0(cr0);

	/* init the fpu */
	finit();

	/* allocate a state-pointer for each cpu (do that just once) */
	if(!curStates) {
		curStates = (XState***)Cache::calloc(SMP::getCPUCount(),sizeof(XState**));
		if(!curStates)
			Util::panic("Unable to allocate memory for FPU-states");
	}
}

void FPU::handleCoProcNA(XState **state) {
	Thread *t = Thread::getRunning();
	XState **current = curStates[t->getCPU()];
	if(current != state) {
		/* if any process has used the FPU in the past */
		if(current != NULL) {
			/* do we have to allocate space for the state? */
			if(*current == NULL) {
				*current = (XState*)Cache::alloc(sizeof(XState));
				/* if we can't save the state, don't unlock the FPU for another process */
				/* TODO ok? */
				if(*current == NULL)
					return;
			}
			/* unlock FPU (necessary here because otherwise fpu_saveState would cause a #NM) */
			CPU::setCR0(CPU::getCR0() & ~CPU::CR0_TASK_SWITCHED);
			/* save state */
			fsave(*current);
		}
		else
			CPU::setCR0(CPU::getCR0() & ~CPU::CR0_TASK_SWITCHED);

		/* init FPU for new process */
		curStates[t->getCPU()] = state;
		if(*state != NULL)
			frestore(*state);
		else
			finit();
	}
	/* just unlock */
	else
		CPU::setCR0(CPU::getCR0() & ~CPU::CR0_TASK_SWITCHED);
}

void FPU::cloneState(XState **dst,const XState *src) {
	if(src != NULL) {
		*dst = (XState*)Cache::alloc(sizeof(XState));
		/* simply ignore it here if alloc fails */
		if(*dst)
			memcpy(*dst,src,sizeof(XState));
	}
	else
		*dst = NULL;
}

void FPU::freeState(XState **state) {
	if(*state != NULL)
		Cache::free(*state);
	*state = NULL;
	/* we have to unset the current state because maybe the next created process gets
	 * the same slot, so that the pointer is the same. */
	for(size_t i = 0, n = SMP::getCPUCount(); i < n; i++) {
		if(curStates[i] == state)
			curStates[i] = NULL;
	}
}
