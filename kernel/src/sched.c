/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../pub/common.h"
#include "../pub/proc.h"
#include "../pub/video.h"
#include "../pub/util.h"

#include "../priv/sched.h"

/* ready-queue stuff */
static sQueueNode readyQueue[PROC_COUNT];
static sQueueNode *rqFree;
static sQueueNode *rqFirst;
static sQueueNode *rqLast;

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
}

sProc *sched_perform(void) {
	sProc *p = proc_getRunning();
	if(p->state == ST_RUNNING) {
		/* put current in the ready-queue */
		p->state = ST_READY;
		sched_enqueueReady(p);
	}

	/* get new process */
	p = sched_dequeueReady();
	/* TODO idle if there is no runnable process */
	p->state = ST_RUNNING;
	return p;
}

void sched_printReadyQueue(void) {
	sQueueNode *n = rqFirst;
	vid_printf("Ready-Queue: rqFirst=0x%x, rqLast=0x%x, rqFree=0x%x\n",rqFirst,rqLast,rqFree);
	while(n != NULL) {
		vid_printf("\t[0x%x]: p=0x%x, next=0x%x\n",n,n->p,n->next);
		n = n->next;
	}
}

void sched_enqueueReady(sProc *p) {
	sQueueNode *nn,*n;
	if(rqFree == NULL)
		panic("No free slots in the ready-queue!?");

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

bool sched_dequeueProc(sProc *p) {
	sQueueNode *n = rqFirst,*l = NULL;
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
