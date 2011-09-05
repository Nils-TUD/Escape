#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "../console.h"
#include "../common.h"
#include "../error.h"
#include "../command.h"
#include "../cpu.h"

#include "trace.h"
#include "tree.h"
#include "map.h"

#define UM(psw)			(((psw) & PSW_UM) ? 1 : 0)
#define PUM(psw)		(((psw) & PSW_PUM) ? 1 : 0)
#define OUM(psw)		(((psw) & PSW_OUM) ? 1 : 0)
#define IE(psw)			(((psw) & PSW_IE) ? 1 : 0)
#define PIE(psw)		(((psw) & PSW_PIE) ? 1 : 0)
#define OIE(psw)		(((psw) & PSW_OIE) ? 1 : 0)

typedef struct {
	/* our root-node for the tree-trace */
	tTNode tRoot;
	/* the last node in the tree-trace */
	tTNode *tLast;
} sTreeTrace;

static sTreeTrace traces[MAX_TRACES];

static Bool tracePrintToFile(FILE *f,char* filename,char* format,...);

/*
 * Inits the tree-trace
 */
void traceTInit(void) {
	int i,j;
	for(j = 0; j < MAX_TRACES; j++) {
		/* init root-node */
		traces[j].tRoot.isEnter = false;
		traces[j].tRoot.isFunc = true;
		traces[j].tRoot.oldPC = 0u;
		traces[j].tRoot.newPC = 0u;
		traces[j].tRoot.psw = 0u;
		for(i = 0; i < 4; i++) {
			traces[j].tRoot.func.args[i] = 0;
		}
		traces[j].tRoot.prev = NULL;
		traces[j].tRoot.next = NULL;
		traces[j].tLast = &traces[j].tRoot;
	}
}

/*
 * Cleans up the tree trace
 */
void traceTClean(void) {
	int j;
	tTNode *n,*nn;
	for(j = 0; j < MAX_TRACES; j++) {
		/* free nodes */
		for(n = traces[j].tRoot.next; n != NULL; n = nn) {
			nn = n->next;
			free(n);
		}
		/* reset */
		traces[j].tRoot.next = NULL;
		traces[j].tLast = &traces[j].tRoot;
	}
}

/*
 * Adds a new node to the tree-trace
 */
void traceTAddNode(tTraceId id,Bool isFunc,int instrCount,Word oldPC,Word newPC,int irq,
		unsigned int *args) {
	tTNode *node;
	int i;

	/* build new node */
	node = (tTNode*)malloc(sizeof(tTNode));
	if(node == NULL) {
		error("cannot allocate trace-node memory");
	}
	node->instrCount = instrCount;
	node->isFunc = isFunc;
	node->isEnter = true;
	node->oldPC = oldPC;
	node->newPC = newPC;
	node->psw = cpuGetPSW();
	if(isFunc) {
		for(i = 0; i < 4; i++) {
			node->func.args[i] = args[i];
		}
	}
	else {
		node->intrpt.irq = irq;
	}
	node->next = NULL;
	node->prev = traces[id].tLast;

	/* add to last node */
	traces[id].tLast->next = node;
	/* we have a new last one :) */
	traces[id].tLast = node;
}

/*
 * Removes the last node from the tree-trace
 */
void traceTRemoveNode(tTraceId id,Bool isFunc,int instrCount,Word oldPC) {
	tTNode *node;
	/* build a new node */
	node = (tTNode*)malloc(sizeof(tTNode));
	if(node == NULL) {
		error("cannot allocate trace-node memory");
	}
	node->instrCount = instrCount;
	node->isFunc = isFunc;
	node->isEnter = false;
	node->oldPC = oldPC;
	node->psw = cpuGetPSW();
	node->next = NULL;
	node->prev = traces[id].tLast;

	/* add to last node */
	traces[id].tLast->next = node;
	/* we have a new last one :) */
	traces[id].tLast = node;
}

/*
 * Prints the tree-trace
 */
void traceTPrint(tTraceId id,char* filename) {
	tTNode *n;
	int x,c = 0,level = 0;
	FILE *f;
	/* open file */
	f = fopen(filename, "w");
    if (f == NULL) {
      cPrintf("cannot open file '%s'\n",filename);
      return;
    }

    /* a little help for the user :) */
    if(!tracePrintToFile(f,filename,
    		"# Format: '<funcName>($4,$5,$6,$7) [<oldPC> -> <newPC>], ic=<InstrCount>'\n")) {
    	return;
    }

	for(n = traces[id].tRoot.next; n != NULL; n = n->next) {
		/* one step back? */
		if(!n->isEnter && level > 0) {
			level--;
		}
		if(level > 1000)
			break;
		/* pad */
		for(x = 0; x < level; x++) {
			if(!tracePrintToFile(f,filename," ")) {
				return;
			}
		}
		/* print function-call */
		if(n->isEnter) {
			if(!tracePrintToFile(f,filename,"\\")) {
				return;
			}
			if(n->isFunc) {
				/* TODO undo */
				if(!tracePrintToFile(f,filename,
						" %s(0x%08x,0x%08x,0x%08x,0x%08x) [0x%08x -> 0x%08x], ic=0x%x, psw=%x, u=%d%d%d,i=%d%d%d\n",
						traceGetFuncName(n->newPC),n->func.args[0],n->func.args[1],n->func.args[2],
						n->func.args[3],n->oldPC,n->newPC,n->instrCount,n->psw,UM(n->psw),PUM(n->psw),
						OUM(n->psw),IE(n->psw),PIE(n->psw),OIE(n->psw))) {
					return;
				}
			}
			else {
				if(!tracePrintToFile(f,filename,
						"-- <Interrupt %d (%s)> [0x%08x -> 0x%08x], ic=0x%x, psw=%x, u=%d%d%d,i=%d%d%d\n",
						n->intrpt.irq,exceptionToString(n->intrpt.irq),n->oldPC,n->newPC,
						n->instrCount,n->psw,UM(n->psw),PUM(n->psw),
						OUM(n->psw),IE(n->psw),PIE(n->psw),OIE(n->psw))) {
					return;
				}
			}
			level += 1;
		}
		/* print return */
		else {
			if(n->isFunc) {
				/*if(!tracePrintToFile(f,filename,"/\n [0x%08X]",n->pc)) {*/
				if(!tracePrintToFile(f,filename,"/ psw=%x, u=%d%d%d,i=%d%d%d\n",n->psw,UM(n->psw),PUM(n->psw),
						OUM(n->psw),IE(n->psw),PIE(n->psw),OIE(n->psw))) {
					return;
				}
			}
			else {
				if(!tracePrintToFile(f,filename,"/-- psw=%x, u=%d%d%d,i=%d%d%d\n",n->psw,UM(n->psw),PUM(n->psw),
						OUM(n->psw),IE(n->psw),PIE(n->psw),OIE(n->psw))) {
					return;
				}
			}
		}
	}

	/* close file */
	fclose(f);
	cPrintf("Stored trace-tree in %s\n");
	/* TODO keep that? */
	fflush(stdout);
}

static Bool tracePrintToFile(FILE *f,char* filename,char* format,...) {
	va_list ap;
	va_start(ap,format);
	if(!vfprintf(f,format,ap)) {
		cPrintf("Unable to write to '%s'\n",filename);
		fclose(f);
		return false;
	}
	va_end(ap);
	return true;
}
