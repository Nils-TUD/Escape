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

#include <common.h>
#include <thread.h>
#include <proc.h>
#include <cpu.h>
#include <gdt.h>
#include <kheap.h>
#include <paging.h>
#include <sched.h>
#include <util.h>
#include <video.h>
#include <sllist.h>
#include <assert.h>
#include <string.h>
#include <errors.h>

#define THREAD_MAP_SIZE		1024

/* our map for the threads. key is (tid % THREAD_MAP_SIZE) */
static sSLList *threadMap[THREAD_MAP_SIZE] = {NULL};
static sThread *cur = NULL;
static tTid nextTid = 0;

/* list of dead threads that should be destroyed */
static sSLList* deadThreads = NULL;

sThread *thread_init(sProc *p) {
	sThread *t = (sThread*)kheap_alloc(sizeof(sThread));
	if(t == NULL)
		util_panic("Unable to allocate mem for initial thread");

	t->state = ST_RUNNING;
	t->events = EV_NOEVENT;
	t->tid = nextTid++;
	t->proc = p;
	/* we'll give the thread a stack later */
	t->ustackBegin = KERNEL_AREA_V_ADDR;
	t->ustackPages = 0;
	t->ucycleCount = 0;
	t->ucycleStart = 0;
	t->kcycleCount = 0;
	t->kcycleStart = 0;
	t->fpuState = NULL;
	t->signal = 0;

	/* create list */
	sSLList *list = threadMap[t->tid % THREAD_MAP_SIZE] = sll_create();
	if(list == NULL)
		util_panic("Unable to allocate mem for initial thread-list");

	/* append */
	if(!sll_append(list,t))
		util_panic("Unable to append initial thread");

	cur = t;
	return t;
}

sThread *thread_getRunning(void) {
	return cur;
}

sThread *thread_getById(tTid tid) {
	sSLNode *n;
	sThread *t;
	sSLList *list = threadMap[tid % THREAD_MAP_SIZE];
	if(list == NULL)
		return NULL;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		t = (sThread*)n->data;
		if(t->tid == tid)
			return t;
	}
	return NULL;
}

void thread_switch(void) {
	thread_switchTo(sched_perform()->tid);
}

void thread_switchTo(tTid tid) {
	sThread *t = thread_getById(tid);
	vassert(t != NULL,"Thread with tid %d not found",tid);

	/* finish kernel-time here since we're switching the process */
	if(cur->kcycleStart > 0)
		cur->kcycleCount += cpu_rdtsc() - cur->kcycleStart;

	if(tid != cur->tid && !thread_save(&cur->save)) {
		/* mark old process ready, if it should not be blocked, killed or something */
		if(cur->state == ST_RUNNING)
			sched_setReady(cur);

		cur = t;
		sched_setRunning(cur);
		/* remove the io-map. it will be set as soon as the process accesses an io-port
		 * (we'll get an exception) */
		/* TODO this is just necessary if the process changes, not just the thread! */
		tss_removeIOMap();
		/* lock the FPU so that we can save the FPU-state for the previous process as soon
		 * as this one wants to use the FPU */
		fpu_lockFPU();
		vid_printf("Resuming %d:%d\n",cur->proc->pid,cur->tid);
		thread_resume(cur->proc->physPDirAddr,&cur->save,cur->kstackFrame);
	}

	/* now start kernel-time again */
	cur->kcycleStart = cpu_rdtsc();

	/* destroy processes, if there is any */
	proc_cleanup();
	/* destroy process, if there is any */
	if(deadThreads != NULL) {
		sSLNode *n;
		for(n = sll_begin(deadThreads); n != NULL; n = n->next) {
			/* we want to destroy the stacks here because if the whole process is destroyed
			 * the proc-module doesn't kill the running thread anyway so there will never
			 * be the case that we should destroy a single thread later */
			thread_destroy((sThread*)n->data,true);
		}
		sll_destroy(deadThreads,false);
		deadThreads = NULL;
	}
}

void thread_wait(tTid tid,u8 events) {
	sThread *t = thread_getById(tid);
	vassert(t != NULL,"Thread with id %d not found",tid);
	t->events = events;
	sched_setBlocked(t);
}

void thread_wakeupAll(u8 event) {
	sched_unblockAll(event);
}

void thread_wakeup(tTid tid,u8 event) {
	sThread *t = thread_getById(tid);
	vassert(t != NULL,"Thread with id %d not found",tid);
	if(t->events & event) {
		/* TODO somehow it is by far slower to use setReadyQuick() here. I can't really
		 * explain this behaviour :/ */
		/*sched_setReadyQuick(p);*/
		sched_setReady(t);
		t->events = EV_NOEVENT;
	}
}

s32 thread_extendStack(u32 address) {
	s32 newPages;
	address &= ~(PAGE_SIZE - 1);
	newPages = ((cur->ustackBegin - address) >> PAGE_SIZE_SHIFT) - cur->ustackPages;
	if(newPages > 0) {
		if(!proc_segSizesValid(cur->proc->textPages,cur->proc->dataPages,cur->proc->stackPages + newPages))
			return ERR_NOT_ENOUGH_MEM;
		if(!proc_changeSize(newPages,CHG_STACK))
			return ERR_NOT_ENOUGH_MEM;
	}
	return 0;
}

s32 thread_clone(sThread *src,sThread **dst,u32 *stackFrame,bool cloneProc) {
	sThread *t = *dst;
	t = (sThread*)kheap_alloc(sizeof(sThread));
	if(t == NULL)
		return ERR_NOT_ENOUGH_MEM;

	t->tid = nextTid++;
	t->state = ST_RUNNING;
	t->events = src->events;
	/* TODO clone fpu-state */
	t->fpuState = NULL;
	t->kcycleCount = 0;
	t->kcycleStart = 0;
	t->ucycleCount = 0;
	t->ucycleStart = 0;
	t->proc = src->proc;
	memcpy(&(t->save),&(src->save),sizeof(src->save));
	t->signal = 0;
	if(cloneProc) {
		/* proc_clone() sets t->kstackFrame in this case */
		t->ustackBegin = src->ustackBegin;
		t->ustackPages = src->ustackPages;
	}
	else {
		/* find an unused stack-beginning. we don't know which stack-begins are occupied because
		 * if we clone a process we clone just the current thread, which is an arbitrary one. And
		 * the stack is kept at the same place. So we have to search which begin is free...
		 */
		sSLNode *n;
		bool ubeginOk;
		u32 ustackBegin = KERNEL_AREA_V_ADDR;
		do {
			ubeginOk = true;
			for(n = sll_begin(src->proc->threads); n != NULL; n = n->next) {
				sThread *pt = (sThread*)n->data;
				if(ustackBegin == pt->ustackBegin) {
					ubeginOk = false;
					break;
				}
			}
			if(!ubeginOk)
				ustackBegin -= MAX_STACK_PAGES * PAGE_SIZE;
		}
		while(!ubeginOk);

		/* create stacks */
		t->ustackBegin = ustackBegin;
		*stackFrame = t->kstackFrame = mm_allocateFrame(MM_DEF);

		/* 2 initial pages; TODO constant */
		t->ustackPages = 2;
		paging_map(t->ustackBegin - t->ustackPages * PAGE_SIZE,NULL,t->ustackPages,PG_WRITABLE,true);
	}

	/* create thread-list if necessary */
	sSLList *list = threadMap[t->tid % THREAD_MAP_SIZE];
	if(list == NULL) {
		list = threadMap[t->tid % THREAD_MAP_SIZE] = sll_create();
		if(list == NULL) {
			kheap_free(t);
			return ERR_NOT_ENOUGH_MEM;
		}
	}

	/* insert */
	if(!sll_append(list,t)) {
		kheap_free(t);
		return ERR_NOT_ENOUGH_MEM;
	}

	*dst = t;
	return 0;
}

void thread_destroy(sThread *t,bool destroyStacks) {
	/* we can't destroy the current thread */
	if(t == cur) {
		/* put it in the dead-thread-queue to destroy it later */
		if(deadThreads == NULL) {
			deadThreads = sll_create();
			if(deadThreads == NULL)
				util_panic("Not enough mem for dead thread-list");
		}
		if(!sll_append(deadThreads,t))
			util_panic("Not enough mem to append dead thread");
		/* remove from scheduler and ensure that he don't picks us again */
		sched_removeThread(t);
		t->state = ST_ZOMBIE;
		return;
	}

	sSLList *list = threadMap[t->tid % THREAD_MAP_SIZE];
	vassert(list != NULL,"Thread %d not found in thread-map",t->tid);

	/* destroy stacks */
	if(destroyStacks)
		paging_destroyStacks(t);

	/* remove from process */
	sProc *p = proc_getRunning();
	p->stackPages -= t->ustackPages;
	sll_removeFirst(p->threads,t);

	/* remove from scheduler and ensure that he don't picks us again */
	sched_removeThread(t);

	/* free FPU-state-memory */
	fpu_freeState(&t->fpuState);
	/* remove signal-handler */
	sig_removeHandlerFor(t->tid);

	sll_removeFirst(list,t);
	kheap_free(t);

	vid_printf("free frames=%d\n",mm_getFreeFrmCount(MM_DEF));
}

/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void thread_dbg_printAll(void) {

}

void thread_dbg_printShort(sThread *t) {
	static const char *states[] = {"UNUSED","RUNNING","READY","BLOCKED","ZOMBIE"};
	vid_printf("\t\t[%d @ %x] state=%s, ustack=0x%08x (%d pages), kstack=%08x\n",
			t->tid,t,states[t->state],t->ustackBegin,t->ustackPages,t->kstackFrame);
}

#endif
