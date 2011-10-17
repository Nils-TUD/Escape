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
#include <sys/task/thread.h>
#include <sys/task/sched.h>
#include <sys/task/timer.h>
#include <sys/mem/vmm.h>
#include <sys/mem/paging.h>
#include <sys/cpu.h>
#include <sys/config.h>
#include <sys/video.h>
#include <esc/sllist.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

/*
 * A few words to cloning and thread-switching on MMIX...
 * Unfortunatly, its a bit more complicated than on ECO32 and i586. There are two problems:
 * 1. The kernel-stack is accessed over the directly mapped space. That means, the address for
 *    every thread is different. Thus, we can't simply copy the kernel-stack, because the stack
 *    might contain pointers to the stack (which can't be distinguished from ordinary integers).
 * 2. It seems to be impossible to save the state somewhere and resume this state later. Because,
 *    when using SAVE the current stack is unusable before we've used UNSAVE to switch to another
 *    context. So, we can't continue after SAVE, but have to do an UNSAVE directly after the SAVE,
 *    i.e. the same assembler routine has to do that to ensure that the the stack is not accessed.
 *
 * The first problem can be solved by the following trick: When cloning, copy the current stack
 * to a temporary page. Then, leave the kernel directly (!) with the parent and make sure that the
 * parent gets a different stack next time when entering the kernel. As soon as the clone wants to
 * run, the stuff on the temporary page is copied to the page that the parent used for the stack
 * at the time we've cloned it. This way, pointers to the stack are no problem, because the same
 * stack-address is used by the clone.
 * Therefore, we introduce archAttr.tempStack to store the frame-number of the temporary stack for
 * a thread. When cloning, copy the stack to it and when the parent returns from thread_initSave,
 * switch the stack with the clone and leave the kernel. But there is still a problem: The
 * SAVE $X,1 at the beginning of a trap uses rSS for the kernel-stack. The corresponding UNSAVE 1,$X
 * uses rSS as well to know the beginning. Thus, we can't simply change rSS for the parent before
 * the UNSAVE 1,$X has been executed. Thus, we have to pass it in some way to the point after UNSAVE.
 * Fortunatly, our rK is always -1 in userspace, i.e. $255 can be directly set before the RESUME 1.
 * Therefore, $255 on the stack (created by SAVE 1,$X) can be used to hold the value of rSS.
 * So, finally, intrpt_forcedTrap does always set $255 on the stack to (2^63 | kstack * PAGE_SIZE),
 * which is put into rSS after UNSAVE.
 * => REALLY inconvenient and probably not very efficient. But I don't see a better solution...
 *
 * The second problem forces us to use a different thread-switch-mechanismn than in ECO32 and i586.
 * Because the old one relies on the opportunity to save the state at any place and resume it later.
 * Since we have to do the SAVE and the RESUME directly after another, we provide the function
 * thread_doSwitchTo and use it when switching the thread. This is no problem in thread_doSwitch()
 * and the initial state can be created by thread_initSave. This one is rather tricky as well. It
 * performs a SAVE at first to write the whole state to memory, copies the register- and software-
 * stack to newStack, writes rWW, ..., rZZ and the end of the stack to saveArea and executes an
 * UNSAVE from the previous SAVE. That means we're doing:
 * SAVE $255,0
 * ... copy ...
 * UNSAVE 0,$255
 * This way, the current stack remains usable and a copy has been created. Of course, we do more
 * than necessary, because most of the values wouldn't need to be restored. The only important ones
 * are rS and rO, which can't be set directly.
 */

extern void thread_startup(void);
extern int thread_initSave(sThreadRegs *saveArea,void *newStack);
extern int thread_doSwitchTo(sThreadRegs *oldArea,sThreadRegs *newArea,tPageDir pdir,tid_t tid);

extern void *stackCopy;
extern uint64_t stackCopySize;
static sThread *cur = NULL;

int thread_initArch(sThread *t) {
	t->archAttr.kstackFrame = pmem_allocate(true);
	if(t->archAttr.kstackFrame < 0)
		return -ENOMEM;
	t->archAttr.tempStack = -1;
	return 0;
}

void thread_addInitialStack(sThread *t) {
	assert(t->tid == INIT_TID);
	t->stackRegions[0] = vmm_add(t->proc->pid,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
			INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACKUP);
	assert(t->stackRegions[0] >= 0);
	t->stackRegions[1] = vmm_add(t->proc->pid,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
			INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACK);
	assert(t->stackRegions[1] >= 0);
}

size_t thread_getThreadFrmCnt(void) {
	return INITIAL_STACK_PAGES * 2;
}

int thread_createArch(const sThread *src,sThread *dst,bool cloneProc) {
	dst->archAttr.kstackFrame = pmem_allocate(true);
	if(dst->archAttr.kstackFrame == 0)
		return -ENOMEM;
	if(!cloneProc) {
		/* add a new stack-region for the register-stack */
		dst->stackRegions[0] = vmm_add(dst->proc->pid,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
				INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACKUP);
		if(dst->stackRegions[0] < 0) {
			pmem_free(dst->archAttr.kstackFrame,true);
			return dst->stackRegions[0];
		}
		/* add a new stack-region for the software-stack */
		dst->stackRegions[1] = vmm_add(dst->proc->pid,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
				INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACK);
		if(dst->stackRegions[1] < 0) {
			/* remove register-stack */
			vmm_remove(dst->proc->pid,dst->stackRegions[0]);
			dst->stackRegions[0] = -1;
			pmem_free(dst->archAttr.kstackFrame,true);
			return dst->stackRegions[1];
		}
	}
	memcpy(dst->archAttr.specRegLevels,src->archAttr.specRegLevels,
			sizeof(sKSpecRegs) * MAX_INTRPT_LEVELS);
	return 0;
}

void thread_freeArch(sThread *t) {
	int i;
	for(i = 0; i < 2; i++) {
		if(t->stackRegions[i] >= 0) {
			vmm_remove(t->proc->pid,t->stackRegions[i]);
			t->stackRegions[i] = -1;
		}
	}
	if(t->archAttr.tempStack != (frameno_t)-1) {
		pmem_free(t->archAttr.tempStack,true);
		t->archAttr.tempStack = -1;
	}
	pmem_free(t->archAttr.kstackFrame,true);
}

sThread *thread_getRunning(void) {
	return cur;
}

void thread_setRunning(sThread *t) {
	cur = t;
}

sKSpecRegs *thread_getSpecRegs(void) {
	sThread *t = thread_getRunning();
	return t->archAttr.specRegLevels + t->intrptLevel - 1;
}

void thread_pushSpecRegs(void) {
	sThread *t = thread_getRunning();
	sKSpecRegs *sregs = t->archAttr.specRegLevels + t->intrptLevel - 1;
	cpu_getKSpecials(&sregs->rbb,&sregs->rww,&sregs->rxx,&sregs->ryy,&sregs->rzz);
}

void thread_popSpecRegs(void) {
	sThread *t = thread_getRunning();
	sKSpecRegs *sregs = t->archAttr.specRegLevels + t->intrptLevel - 1;
	cpu_setKSpecials(sregs->rbb,sregs->rww,sregs->rxx,sregs->ryy,sregs->rzz);
}

int thread_finishClone(sThread *t,sThread *nt) {
	int res;
	nt->archAttr.tempStack = pmem_allocate(true);
	if(nt->archAttr.tempStack == 0)
		return -ENOMEM;
	res = thread_initSave(&nt->save,(void*)(DIR_MAPPED_SPACE | (nt->archAttr.tempStack * PAGE_SIZE)));
	if(res == 0) {
		/* the parent needs a new kernel-stack for the next kernel-entry */
		/* switch stacks */
		frameno_t kstack = t->archAttr.kstackFrame;
		t->archAttr.kstackFrame = nt->archAttr.kstackFrame;
		nt->archAttr.kstackFrame = kstack;
	}
	return res;
}

void thread_finishThreadStart(A_UNUSED sThread *t,sThread *nt,const void *arg,uintptr_t entryPoint) {
	/* copy stack of kernel-start */
	uint64_t *ssp,*rsp = (uint64_t*)(DIR_MAPPED_SPACE | (nt->archAttr.kstackFrame * PAGE_SIZE));
	uintptr_t start = (uintptr_t)rsp;
	memcpy(rsp,&stackCopy,stackCopySize);
	rsp += stackCopySize / sizeof(uint64_t) - 1;
	rsp[-8] = (uint64_t)&thread_startup;							/* rJ = startup-code */
	/* store stack-end for UNSAVE */
	nt->save.stackEnd = (uintptr_t)rsp;
	/* put arguments for the startup-code on the software-stack */
	ssp = (uint64_t*)(start + PAGE_SIZE - sizeof(uint64_t));
	*ssp-- = entryPoint;
	*ssp-- = (uint64_t)arg;
	*ssp = nt->proc->entryPoint;
	/* set sp and fp */
	rsp[-(13 + 1)] = (uint64_t)ssp;									/* $254 */
	rsp[-(13 + 2)] = (uint64_t)ssp;									/* $253 */
	/* no tempstack here */
	nt->archAttr.tempStack = -1;
}

uint64_t thread_getRuntime(const sThread *t) {
	if(t->state == ST_RUNNING) {
		/* if the thread is running, we must take the time since the last scheduling of that thread
		 * into account. this is especially a problem with idle-threads */
		uint64_t cycles = cpu_rdtsc();
		return (t->stats.runtime + timer_cyclesToTime(cycles - t->stats.cycleStart));
	}
	return t->stats.runtime;
}

bool thread_isRunning(sThread *t) {
	return t->state == ST_RUNNING;
}

void thread_doSwitch(void) {
	uint64_t cycles,runtime;
	sThread *old = thread_getRunning();
	sThread *new;

	/* update runtime-stats */
	cycles = cpu_rdtsc();
	runtime = timer_cyclesToTime(cycles - old->stats.cycleStart);
	old->stats.runtime += runtime;
	old->stats.curCycleCount += cycles - old->stats.cycleStart;

	/* choose a new thread to run */
	new = sched_perform(old,runtime);
	new->stats.schedCount++;

	/* switch thread */
	if(new->tid != old->tid) {
		thread_setRunning(new);

		if(conf_getStr(CONF_SWAP_DEVICE)) {
			time_t timestamp = timer_getTimestamp();
			vmm_setTimestamp(new,timestamp);
		}

		/* if we still have a temp-stack, copy the contents to our real stack and free the
		 * temp-stack */
		if(new->archAttr.tempStack != (frameno_t)-1) {
			memcpy((void*)(DIR_MAPPED_SPACE | new->archAttr.kstackFrame * PAGE_SIZE),
					(void*)(DIR_MAPPED_SPACE | new->archAttr.tempStack * PAGE_SIZE),
					PAGE_SIZE);
			pmem_free(new->archAttr.tempStack,true);
			new->archAttr.tempStack = -1;
		}

		/* TODO we have to clear the TCs if the process shares its address-space with another one */
		new->stats.cycleStart = cpu_rdtsc();
		thread_doSwitchTo(&old->save,&new->save,new->proc->pagedir,new->tid);
	}
	else
		new->stats.cycleStart = cpu_rdtsc();
}


#if DEBUGGING

void thread_printState(const sThreadRegs *state) {
	vid_printf("\tState:\n",state);
	vid_printf("\t\tStackend = %p\n",state->stackEnd);
}

#endif
