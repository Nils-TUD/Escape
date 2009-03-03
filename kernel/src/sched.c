/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/sched.h"
#include "../h/proc.h"
#include "../h/util.h"
#include <video.h>
#include <sllist.h>

/* the queue for all runnable (but not currently running) processes */
typedef struct sQueueNode sQueueNode;
struct sQueueNode {
	sQueueNode *next;
	sProc *p;
};

/* ready-queue stuff */
static sQueueNode readyQueue[PROC_COUNT];
static sQueueNode *rqFree;
static sQueueNode *rqFirst;
static sQueueNode *rqLast;

static sSLList *blockedQueue;

void sched_init(void) {
	s32 i;
	sQueueNode *node;
	/* put all on the free-queue */
	node = &readyQueue[PROC_COUNT - 1];
	node->next = NULL;
	node--;
	for(i = PROC_COUNT - 2; i >= 0; i--) {
		node->next = node + 1;
		node--;
	}
	/* all free atm */
	rqFree = &readyQueue[0];
	rqFirst = NULL;
	rqLast = NULL;

	/* init sleep-queue */
	blockedQueue = sll_create();
}

sProc *sched_perform(void) {
	sProc *p = proc_getRunning();
	if(p->state == ST_RUNNING) {
		/* put current in the ready-queue */
		sched_setReady(p);
	}

	/* get new process */
	p = sched_dequeueReady();
	/* TODO idle if there is no runnable process */
	p->state = ST_RUNNING;
	return p;
}

void sched_setBlocked(sProc *p) {
	ASSERT(p != NULL,"p == NULL");

	/* nothing to do? */
	if(p->state == ST_BLOCKED)
		return;
	if(p->state == ST_READY)
		sched_dequeueReadyProc(p);
	p->state = ST_BLOCKED;

	/* insert in blocked-list */
	sll_append(blockedQueue,p);
}

void sched_setReady(sProc *p) {
	sQueueNode *nn,*n;

	ASSERT(p != NULL,"p == NULL");
	ASSERT(rqFree != NULL,"No free slots in the ready-queue!?");

	/* nothing to do? */
	if(p->state == ST_READY)
		return;
	/* remove from blocked-list? */
	if(p->state == ST_BLOCKED)
		sll_removeFirst(blockedQueue,p);
	p->state = ST_READY;

	/* use first free node */
	nn = rqFree;
	rqFree = rqFree->next;
	nn->p = p;
	nn->next = NULL;

	/* put at the end of the ready-queue */
	n = rqFirst;
	if(n != NULL) {
		rqLast->next = nn;
		rqLast = nn;
	}
	else {
		rqFirst = nn;
		rqLast = nn;
	}
}

sProc *sched_dequeueReady(void) {
	sQueueNode *node;
	if(rqFirst == NULL)
		return NULL;

	/* put in free-queue & remove from ready-queue */
	node = rqFirst;
	rqFirst = rqFirst->next;
	if(rqFirst == NULL)
		rqLast = NULL;
	node->next = rqFree;
	rqFree = node;

	return node->p;
}

void sched_removeProc(sProc *p) {
	switch(p->state) {
		case ST_READY:
			sched_dequeueReadyProc(p);
			break;
		case ST_BLOCKED:
			sll_removeFirst(blockedQueue,p);
			break;
	}
}

bool sched_dequeueReadyProc(sProc *p) {
	sQueueNode *n = rqFirst,*l = NULL;

	ASSERT(p != NULL,"p == NULL");

	while(n != NULL) {
		/* found it? */
		if(n->p == p) {
			/* dequeue */
			if(l == NULL)
				l = rqFirst = n->next;
			else
				l->next = n->next;
			if(n->next == NULL)
				rqLast = l;
			n->next = rqFree;
			rqFree = n;
			return true;
		}
		/* to next */
		l = n;
		n = n->next;
	}
	return false;
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void sched_dbg_print(void) {
	sQueueNode *n = rqFirst;
	vid_printf("Ready-Queue: rqFirst=0x%x, rqLast=0x%x, rqFree=0x%x\n",rqFirst,rqLast,rqFree);
	while(n != NULL) {
		vid_printf("\t[0x%x]: p=0x%x, next=0x%x\n",n,n->p,n->next);
		n = n->next;
	}
	vid_printf("Blocked-queue:\n");
	sll_dbg_print(blockedQueue);
	vid_printf("\n");
}

#endif
