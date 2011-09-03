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
#include <errors.h>

extern void thread_startup(void);
extern bool thread_save(sThreadRegs *saveArea);
extern bool thread_resume(tPageDir pageDir,const sThreadRegs *saveArea,frameno_t kstackFrame);

static sThread *cur = NULL;

int thread_initArch(sThread *t) {
	/* setup kernel-stack for us */
	frameno_t stackFrame = pmem_allocate();
	paging_mapTo(&t->proc->pagedir,KERNEL_STACK,&stackFrame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	t->archAttr.kstackFrame = stackFrame;
	return 0;
}

void thread_addInitialStack(sThread *t) {
	assert(t->tid == INIT_TID);
	t->stackRegions[0] = vmm_add(t->proc->pid,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
			INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACK);
	assert(t->stackRegions[0] >= 0);
}

int thread_createArch(const sThread *src,sThread *dst,bool cloneProc) {
	UNUSED(src);
	if(cloneProc)
		thread_initArch(dst);
	else {
		if(pmem_getFreeFrames(MM_DEF) < INITIAL_STACK_PAGES)
			return ERR_NOT_ENOUGH_MEM;

		dst->archAttr.kstackFrame = pmem_allocate();

		/* add a new stack-region */
		dst->stackRegions[0] = vmm_add(dst->proc->pid,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
				INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACK);
		if(dst->stackRegions[0] < 0) {
			pmem_free(dst->archAttr.kstackFrame);
			return dst->stackRegions[0];
		}
	}
	return 0;
}

void thread_freeArch(sThread *t) {
	if(t->stackRegions[0] >= 0) {
		vmm_remove(t->proc->pid,t->stackRegions[0]);
		t->stackRegions[0] = -1;
	}
	pmem_free(t->archAttr.kstackFrame);
}

sThread *thread_getRunning(void) {
	return cur;
}

void thread_setRunning(sThread *t) {
	cur = t;
}

int thread_finishClone(sThread *t,sThread *nt) {
	UNUSED(t);
	ulong *src;
	size_t i;
	/* we clone just the current thread. all other threads are ignored */
	/* map stack temporary (copy later) */
	ulong *dst = (ulong*)(DIR_MAPPED_SPACE | (nt->archAttr.kstackFrame * PAGE_SIZE));

	if(thread_save(&nt->save)) {
		/* child */
		return 1;
	}

	/* now copy the stack */
	/* copy manually to prevent a function-call (otherwise we would change the stack) */
	src = (ulong*)KERNEL_STACK;
	for(i = 0; i < PT_ENTRY_COUNT; i++)
		*dst++ = *src++;

	/* parent */
	return 0;
}

void thread_finishThreadStart(sThread *t,sThread *nt,const void *arg,uintptr_t entryPoint) {
	UNUSED(t);
	/* prepare registers for the first thread_resume() */
	nt->save.r16 = nt->proc->entryPoint;
	nt->save.r17 = entryPoint;
	nt->save.r18 = (uint32_t)arg;
	nt->save.r19 = 0;
	nt->save.r20 = 0;
	nt->save.r21 = 0;
	nt->save.r22 = 0;
	nt->save.r23 = 0;
	nt->save.r25 = 0;
	nt->save.r29 = KERNEL_STACK + PAGE_SIZE - sizeof(int);
	nt->save.r30 = 0;
	nt->save.r31 = (uint32_t)&thread_startup;
}

uint64_t thread_getRuntime(const sThread *t) {
	return t->stats.runtime;
}

bool thread_isRunning(sThread *t) {
	return t->state == ST_RUNNING;
}

void thread_doSwitch(void) {
	sThread *old = thread_getRunning();
	sThread *new = sched_perform(old);
	/* finish kernel-time here since we're switching the process */
	if(new->tid != old->tid) {
		/* eco32 has no cycle-counter or similar. therefore we use the timer for runtime-
		 * measurement */
		time_t timestamp = timer_getTimestamp();
		old->stats.runtime += (timestamp - old->stats.cycleStart) * 1000;
		new->stats.schedCount++;

		if(!thread_save(&old->save)) {
			thread_setRunning(new);

			if(conf_getStr(CONF_SWAP_DEVICE) != NULL)
				vmm_setTimestamp(new,timestamp);

			new->stats.cycleStart = timestamp;
			thread_resume(new->proc->pagedir,&new->save,new->archAttr.kstackFrame);
		}
	}
}


#if DEBUGGING

void thread_printState(const sThreadRegs *state) {
	vid_printf("\tState:\n",state);
	vid_printf("\t\t$16 = %#08x\n",state->r16);
	vid_printf("\t\t$17 = %#08x\n",state->r17);
	vid_printf("\t\t$18 = %#08x\n",state->r18);
	vid_printf("\t\t$19 = %#08x\n",state->r19);
	vid_printf("\t\t$20 = %#08x\n",state->r20);
	vid_printf("\t\t$21 = %#08x\n",state->r21);
	vid_printf("\t\t$22 = %#08x\n",state->r22);
	vid_printf("\t\t$23 = %#08x\n",state->r23);
	vid_printf("\t\t$29 = %#08x\n",state->r29);
	vid_printf("\t\t$30 = %#08x\n",state->r30);
	vid_printf("\t\t$31 = %#08x\n",state->r31);
}

#endif
