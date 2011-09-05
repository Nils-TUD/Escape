#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "mmix/mem.h"
#include "mmix/io.h"
#include "getline/getline.h"
#include "cli/lang/bt.h"
#include "cli/lang/symmap.h"
#include "cli/lang/btsimple.h"
#include "exception.h"

#define STACK_LIMIT	100

typedef struct {
	/* our root-node for the simple-trace */
	tSNode sRoot;
	/* the last node in the simple-trace */
	tSNode *sLast;
	/* the current stack-level */
	int sLevel;
} sSimpleTrace;

static sSimpleTrace traces[MAX_TRACES];

int btsimple_getStackLevel(tTraceId id) {
	return traces[id].sLevel;
}

void btsimple_init(void) {
	for(size_t j = 0; j < MAX_TRACES; j++) {
		/* init root-node */
		traces[j].sRoot.data.oldPC = 0u;
		traces[j].sRoot.data.newPC = 0u;
		traces[j].sRoot.data.d.func.argCount = 0;
		traces[j].sRoot.prev = NULL;
		traces[j].sRoot.next = NULL;
		traces[j].sLast = &traces[j].sRoot;
		traces[j].sLevel = 0;
	}
}

void btsimple_clean(tTraceId id) {
	tSNode *n,*nn;
	/* free nodes */
	for(n = traces[id].sRoot.next; n != NULL; n = nn) {
		nn = n->next;
		mem_free(n);
	}
	/* reset */
	traces[id].sRoot.next = NULL;
	traces[id].sLast = &traces[id].sRoot;
	traces[id].sLevel = 0;
}

void btsimple_cleanAll(void) {
	for(size_t j = 0; j < MAX_TRACES; j++)
		btsimple_clean(j);
}

void btsimple_add(tTraceId id,bool isFunc,octa instrCount,octa oldPC,octa newPC,octa threadId,
		int ex,octa bits,size_t argCount,octa *args) {
	tSNode *node;

	/* build new node */
	node = (tSNode*)mem_alloc(sizeof(tSNode));
	bt_fillForAdd(&node->data,isFunc,instrCount,oldPC,newPC,threadId,ex,bits,argCount,args);
	node->next = NULL;
	node->prev = traces[id].sLast;
	traces[id].sLevel++;

	/* add to last node */
	traces[id].sLast->next = node;
	/* we have a new last one :) */
	traces[id].sLast = node;
}

void btsimple_remove(tTraceId id,bool isFunc,octa instrCount,octa oldPC,octa threadId) {
	UNUSED(isFunc);
	UNUSED(instrCount);
	UNUSED(oldPC);
	UNUSED(threadId);
	tSNode *n = traces[id].sLast;
	if(n != &traces[id].sRoot) {
		traces[id].sLast = traces[id].sLast->prev;
		traces[id].sLast->next = NULL;
		mem_free(n);
		traces[id].sLevel--;
	}
}

void btsimple_print(tTraceId id) {
	sSimpleTrace *t = traces + id;
	tSNode *n = t->sRoot.next;
	int x,level = 0;
	if(n == NULL) {
		mprintf("- Trace is empty -\n");
	}
	else {
		if(t->sLevel > STACK_LIMIT) {
			char *line;
			mprintf("The stack is %d calls deep.",t->sLevel);
			mprintf(" Are you sure you want to display it?\n");
			line = gl_getline((char*)"[y/n]: ");
			if(*line != 'y')
				return;
		}

		mprintf("%% Format: '<funcName>(<args>) [<oldPC> -> <newPC>]'\n");
		for(; n != NULL; n = n->next) {
			/* pad */
			for(x = 0; x < level; x++) {
				mprintf(" ");
			}
			mprintf("\\ ");
			if(n->data.isFunc) {
				mprintf("%s(",symmap_getFuncName(n->data.newPC));
				for(size_t i = 0; i < n->data.d.func.argCount; i++) {
					mprintf("#%OX",n->data.d.func.args[i]);
					if(i < n->data.d.func.argCount - 1)
						mprintf(",");
				}
				mprintf(") [#%OX -> #%OX]\n",n->data.oldPC,n->data.newPC);
			}
			else {
				const char *ex = ex_toString(n->data.d.intrpt.ex,n->data.d.intrpt.bits);
				mprintf("-- <%s> [0x%016OX -> 0x%016OX]\n",ex,n->data.oldPC,n->data.newPC);
			}
			level += 1;
		}
	}
	/* TODO keep that? */
	fflush(stdout);
}
