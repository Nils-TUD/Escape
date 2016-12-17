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

#include <arch/x86/fpu.h>
#include <mem/cache.h>
#include <task/smp.h>
#include <task/thread.h>
#include <common.h>
#include <cpu.h>
#include <string.h>
#include <util.h>
#include <video.h>

Thread **FPU::curStates = NULL;
SpinLock FPU::lock;

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
		curStates = (Thread**)Cache::calloc(SMP::getCPUCount(),sizeof(Thread*));
		if(!curStates)
			Util::panic("Unable to allocate memory for FPU-states");
	}
}

void FPU::handleCoProcNA(Thread *t) {
	LockGuard<SpinLock> guard(&lock);

	/* were we migrated? */
	if(t->getFlags() & T_FPU_WAIT) {
		/* wait until the old CPU has saved the state */
		t->getFPUSem().down(&lock);
		t->setFlags(t->getFlags() & ~T_FPU_WAIT);
	}

	Thread *current = curStates[t->getCPU()];
	if(current != t) {
		/* init FPU for new process */
		curStates[t->getCPU()] = t;

		/* if any process has used the FPU in the past */
		if(current != NULL) {
			/* TODO handle that case */
			if(!doSave(current->getFPUStatePtr()))
				Util::panic("Unable to save FPU state");

			/* notify thread that the state is saved now */
			if(current->getFlags() & T_FPU_SIGNAL) {
				current->setFlags(current->getFlags() & ~T_FPU_SIGNAL);
				current->getFPUSem().up();
			}
		}
		else
			CPU::setCR0(CPU::getCR0() & ~CPU::CR0_TASK_SWITCHED);

		XState **state = t->getFPUStatePtr();
		if(*state != NULL)
			frestore(*state);
		else
			finit();
	}
	/* just unlock */
	else
		CPU::setCR0(CPU::getCR0() & ~CPU::CR0_TASK_SWITCHED);
}

void FPU::initSaveState(Thread *t) {
	if(curStates[t->getCPU()] == t) {
		LockGuard<SpinLock> guard(&lock);
		/* only do that if we did not request a remote-save yet */
		if(curStates[t->getCPU()] == t && !(t->getFlags() & T_FPU_WAIT)) {
			/* remember that we were migrated */
			t->setFlags(t->getFlags() | T_FPU_SIGNAL | T_FPU_WAIT);
			/* send an IPI to the old CPU to save the FPU state */
			SMP::saveFPU(t->getCPU());
		}
	}
}

void FPU::saveState(Thread *t) {
	LockGuard<SpinLock> guard(&lock);
	Thread *current = curStates[t->getCPU()];
	/* do the save&signal just once; maybe it was already done in handleCoProcNA */
	if(current && (current->getFlags() & T_FPU_SIGNAL)) {
		/* TODO how to handle an error here? */
		if(!doSave(current->getFPUStatePtr()))
			Util::panic("Unable to save FPU state");

		/* forget state */
		curStates[t->getCPU()] = NULL;
		/* notify thread that the state is saved now */
		current->setFlags(current->getFlags() & ~T_FPU_SIGNAL);
		current->getFPUSem().up();

		/* lock FPU again */
		CPU::setCR0(CPU::getCR0() | CPU::CR0_TASK_SWITCHED);
	}
}

bool FPU::doSave(XState **current) {
	/* do we have to allocate space for the state? */
	if(*current == NULL) {
		*current = new XState;
		if(*current == NULL)
			return false;
	}
	/* unlock FPU (necessary here because otherwise fpu_saveState would cause a #NM) */
	CPU::setCR0(CPU::getCR0() & ~CPU::CR0_TASK_SWITCHED);
	/* save state */
	fsave(*current);
	return true;
}

void FPU::cloneState(Thread *dst,const Thread *src) {
	LockGuard<SpinLock> guard(&lock);
	if(src->getFPUState() != NULL) {
		XState *state = *dst->getFPUStatePtr() = new XState;
		/* simply ignore it here if alloc fails */
		if(state)
			memcpy(state,src->getFPUState(),sizeof(*state));
	}
	else
		*dst->getFPUStatePtr() = NULL;
}

void FPU::freeState(Thread *t) {
	LockGuard<SpinLock> guard(&lock);
	XState **state = t->getFPUStatePtr();
	if(*state != NULL)
		delete *state;
	*state = NULL;
	/* we have to unset the current state because maybe the next created process gets
	 * the same slot, so that the pointer is the same. */
	for(size_t i = 0, n = SMP::getCPUCount(); i < n; i++) {
		if(curStates[i] == t)
			curStates[i] = NULL;
	}
}
