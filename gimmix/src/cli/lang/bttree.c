#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "common.h"
#include "mmix/mem.h"
#include "mmix/io.h"
#include "getline/getline.h"
#include "cli/lang/bt.h"
#include "cli/lang/bttree.h"
#include "cli/lang/symmap.h"
#include "core/cpu.h"
#include "exception.h"

#define MAX_NODES_PER_TREE	1000000

typedef struct {
	/* number of nodes in this tree */
	unsigned long count;
	/* our root-node for the tree-trace */
	tTNode tRoot;
	/* the last node in the tree-trace */
	tTNode *tLast;
} sTreeTrace;

static void bttree_checkSize(tTraceId id);

static sTreeTrace traces[MAX_TRACES];

void bttree_init(void) {
	for(size_t j = 0; j < MAX_TRACES; j++) {
		/* init root-node */
		traces[j].count = 0;
		traces[j].tRoot.isEnter = false;
		traces[j].tRoot.data.isFunc = true;
		traces[j].tRoot.data.oldPC = 0u;
		traces[j].tRoot.data.newPC = 0u;
		traces[j].tRoot.data.d.func.argCount = 0;
		traces[j].tRoot.prev = NULL;
		traces[j].tRoot.next = NULL;
		traces[j].tLast = &traces[j].tRoot;
	}
}

void bttree_clean(tTraceId id) {
	tTNode *n,*nn;
	/* free nodes */
	for(n = traces[id].tRoot.next; n != NULL; n = nn) {
		nn = n->next;
		mem_free(n);
	}
	/* reset */
	traces[id].tRoot.next = NULL;
	traces[id].tLast = &traces[id].tRoot;
	traces[id].count = 0;
}

void bttree_cleanAll(void) {
	for(size_t j = 0; j < MAX_TRACES; j++)
		bttree_clean(j);
}

void bttree_add(tTraceId id,bool isFunc,octa instrCount,octa oldPC,octa newPC,octa threadId,
		int ex,octa bits,size_t argCount,octa *args) {
	tTNode *node;
	bttree_checkSize(id);

	/* build new node */
	node = (tTNode*)mem_alloc(sizeof(tTNode));
	bt_fillForAdd(&node->data,isFunc,instrCount,oldPC,newPC,threadId,ex,bits,argCount,args);
	node->isEnter = true;
	node->next = NULL;
	node->prev = traces[id].tLast;

	/* add to last node */
	traces[id].tLast->next = node;
	/* we have a new last one :) */
	traces[id].tLast = node;
	traces[id].count++;
}

void bttree_remove(tTraceId id,bool isFunc,octa instrCount,octa oldPC,octa threadId) {
	tTNode *node;
	bttree_checkSize(id);

	/* build a new node */
	node = (tTNode*)mem_alloc(sizeof(tTNode));
	bt_fillForRemove(&node->data,isFunc,instrCount,oldPC,threadId);
	node->isEnter = false;
	node->next = NULL;
	node->prev = traces[id].tLast;

	/* add to last node */
	traces[id].tLast->next = node;
	/* we have a new last one :) */
	traces[id].tLast = node;
	traces[id].count++;
}

void bttree_print(tTraceId id,const char *filename) {
	tTNode *n;
	int x,level = 0;
	FILE *f;
	/* open file */
	f = fopen(filename,"w");
	if(f == NULL) {
		mprintf("cannot open file '%s'\n",filename);
		return;
	}

	for(n = traces[id].tRoot.next; n != NULL; n = n->next) {
		/* one step back? */
		if(!n->isEnter && level > 0)
			level--;
		if(level > 1000)
			break;
		/* pad */
		for(x = 0; x < level; x++)
			mfprintf(f," ");

		/* print function-call */
		if(n->isEnter) {
			mfprintf(f,"\\");
			if(n->data.isFunc) {
				mfprintf(f," %s(",symmap_getFuncName(n->data.newPC));
				for(size_t i = 0; i < n->data.d.func.argCount; i++) {
					mfprintf(f,"#%OX",n->data.d.func.args[i]);
					if(i < n->data.d.func.argCount - 1)
						mfprintf(f,",");
				}
				mfprintf(f,") [#%OX -> #%OX], t=%d, ic=#%OX\n",
						n->data.oldPC,n->data.newPC,n->data.threadId,n->data.instrCount);
			}
			else {
				const char *ex = ex_toString(n->data.d.intrpt.ex,n->data.d.intrpt.bits);
				mfprintf(f,"-- <%s> [#%OX -> #%OX], t=%d, ic=#%OX\n",ex,n->data.oldPC,n->data.newPC,
						n->data.threadId,n->data.instrCount);
			}
			level += 1;
		}
		/* print return */
		else {
			if(n->data.isFunc)
				mfprintf(f,"/ t=%d, ic=#%OX\n",n->data.threadId,n->data.instrCount);
			else
				mfprintf(f,"/-- t=%d, ic=#%OX\n",n->data.threadId,n->data.instrCount);
		}
	}

	/* close file */
	fclose(f);
	mprintf("Stored trace-tree in %s\n",filename);
	/* TODO keep that? */
	fflush(stdout);
}

static void bttree_checkSize(tTraceId id) {
	/* remove the first node, if our tree is full */
	if(traces[id].count == MAX_NODES_PER_TREE) {
		tTNode *node = traces[id].tRoot.next;
		traces[id].tRoot.next = node->next;
		mem_free(node);
		traces[id].count--;
	}
}
