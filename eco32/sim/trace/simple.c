#include <stdlib.h>
#include <stdio.h>

#include "../console.h"
#include "../common.h"
#include "../error.h"
#include "../command.h"

#include "trace.h"
#include "map.h"
#include "simple.h"

#define STACK_LIMIT	32

typedef struct {
	/* our root-node for the simple-trace */
	tSNode sRoot;
	/* the last node in the simple-trace */
	tSNode *sLast;
	/* the current stack-level */
	int sLevel;
} sSimpleTrace;

static sSimpleTrace traces[MAX_TRACES];

/*
 * Returns the current stack-level, that means the depth in our current call-hierarchie
 */
int traceSGetStackLevel(tTraceId id) {
	return traces[id].sLevel;
}

/*
 * Inits the simple-trace
 */
void traceSInit(void) {
	int j,i;
	for(j = 0; j < MAX_TRACES; j++) {
		/* init root-node */
		traces[j].sRoot.oldPC = 0u;
		traces[j].sRoot.newPC = 0u;
		for(i = 0; i < 4; i++) {
			traces[j].sRoot.func.args[i] = 0;
		}
		traces[j].sRoot.prev = NULL;
		traces[j].sRoot.next = NULL;
		traces[j].sLast = &traces[j].sRoot;
		traces[j].sLevel = 0;
	}
}

/*
 * Cleans up the simple trace
 */
void traceSClean(void) {
	int j;
	tSNode *n,*nn;
	for(j = 0; j < MAX_TRACES; j++) {
		/* free nodes */
		for(n = traces[j].sRoot.next; n != NULL; n = nn) {
			nn = n->next;
			free(n);
		}
		/* reset */
		traces[j].sRoot.next = NULL;
		traces[j].sLast = &traces[j].sRoot;
		traces[j].sLevel = 0;
	}
}

/*
 * Adds a new node to the simple-trace
 */
void traceSAddNode(tTraceId id,Bool isFunc,int instrCount,Word oldPC,Word newPC,int irq,
		unsigned int *args) {
	tSNode *node;
	int i;

	/* build new node */
	node = (tSNode*)malloc(sizeof(tSNode));
	if(node == NULL) {
		error("cannot allocate trace-node memory");
	}
	node->instrCount = instrCount;
	node->isFunc = isFunc;
	node->oldPC = oldPC;
	node->newPC = newPC;
	if(isFunc) {
		for(i = 0; i < 4; i++) {
			node->func.args[i] = args[i];
		}
	}
	else {
		node->intrpt.irq = irq;
	}
	node->next = NULL;
	node->prev = traces[id].sLast;
	traces[id].sLevel++;

	/* add to last node */
	traces[id].sLast->next = node;
	/* we have a new last one :) */
	traces[id].sLast = node;
}

/*
 * Removes the last node from the simple-trace
 */
void traceSRemoveNode(tTraceId id,Bool isFunc,int instrCount,Word oldPC) {
	tSNode *n = traces[id].sLast;
	if(n != &traces[id].sRoot) {
		traces[id].sLast = traces[id].sLast->prev;
		traces[id].sLast->next = NULL;
		free(n);
		traces[id].sLevel--;
	}
}

/*
 * Prints the simple-trace
 */
void traceSPrint(tTraceId id) {
	sSimpleTrace *t = traces + id;
	tSNode *n = t->sRoot.next;
	int x,level = 0;
	if(n == NULL) {
		cPrintf("- Trace is empty -\n");
	}
	else {
		cPrintf("# Format: '<funcName>($4,$5,$6,$7) [<oldPC> -> <newPC>]'\n");
		for(; n != NULL; n = n->next) {
			if(level > STACK_LIMIT) {
				cPrintf("*** Limit reached; stopping here ***\n");
				break;
			}
			/* pad */
			for(x = 0; x < level; x++) {
				cPrintf(" ");
			}
			cPrintf("\\ ");
			if(n->isFunc) {
				cPrintf("%s(0x%08x,0x%08x,0x%08x,0x%08x) [0x%08x -> 0x%08x]\n",
					traceGetFuncName(n->newPC),n->func.args[0],n->func.args[1],n->func.args[2],
					n->func.args[3],n->oldPC,n->newPC);
			}
			else {
				cPrintf("<Interrupt %d (%s)> [0x%08x -> 0x%08x]\n",n->intrpt.irq,
						exceptionToString(n->intrpt.irq),n->oldPC,n->newPC);
			}
			level += 1;
		}
	}
	/* TODO keep that? */
	fflush(stdout);
}
