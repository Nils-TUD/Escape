/**
 * @version		$Id: proc.h 48 2008-11-14 20:32:43Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/sched.h"
#include "../h/proc.h"
#include "../h/video.h"
#include "../h/util.h"

/* the queue for all runnable (but not currently running) processes */
typedef struct tQueueNode tQueueNode;
struct tQueueNode {
	tQueueNode *next;
	tProc *p;
};

/* ready-queue stuff */
static tQueueNode readyQueue[PROC_COUNT];
static tQueueNode *rqFree;
static tQueueNode *rqFirst;
static tQueueNode *rqLast;

void sched_init(void) {
	s32 i;
	tQueueNode *node;
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

tProc *sched_perform(void) {
	tProc *p = proc_getRunning();
	/* put current in the ready-queue */
	p->state = ST_READY;
	sched_enqueueReady(p);

	/* get new process */
	p = sched_dequeueReady();
	p->state = ST_RUNNING;
	return p;
}

void sched_printReadyQueue(void) {
	tQueueNode *n = rqFirst;
	vid_printf("Ready-Queue:\n");
	while(n != NULL) {
		vid_printf("\tpid=%d, next=0x%x\n",n->p->pid,n->next);
		n = n->next;
	}
}

void sched_enqueueReady(tProc *p) {
	tQueueNode *nn,*n;
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

void sched_dequeueProc(tProc *p) {
	tQueueNode *n = rqFirst,*l = NULL;
	while(n != NULL) {
		/* found it? */
		if(n->p == p) {
			/* dequeue */
			l->next = n->next;
			n->next = rqFree;
			rqFree = n;
			break;
		}
		/* to next */
		l = n;
		n = n->next;
	}
}

tProc *sched_dequeueReady(void) {
	tQueueNode *node;
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
