#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../common.h"
#include "../error.h"
#include "../cpu.h"
#include "../command.h"
#include "../console.h"
#include "../mmu.h"

#include "trace.h"
#include "simple.h"
#include "tree.h"
#include "map.h"

#define TRACES_PER_LINE		5

/* Tracing may be disabled. This makes sense for example if it would consume too much
 * memory */
Bool tracingEnabled = false;
Bool treeEnabled = true;

/* our traces */
static int traceCount = 1;			/* the first trace is "all" */
static Word traces[MAX_TRACES];

static tTraceId getTraceId(void);
static tTraceId readTraceId(void);

/*
 * Inits the trace-mechanism
 */
void traceInit(char* mapNames[]) {
	int i;
	for(i = 0; i < MAX_TRACES; i++)
		traces[i] = 0xC0000000;

	if (mapNames[0] != NULL) {
		tracingEnabled = true;
		for(i = 0; i < MAX_MAPS; i++) {
			if(mapNames[i] == NULL) {
				break;
			}
			traceParseMap(mapNames[i]);
		}
		if (!tracingEnabled) {
			return;
		}
		traceSInit();
		if(treeEnabled)
			traceTInit();
	}
}

/*
 * Resets the trace-mechanism
 */
void traceReset(void) {
	if(tracingEnabled) {
		traceSClean();
		if(treeEnabled)
			traceTClean();
		traceSInit();
		if(treeEnabled)
			traceTInit();
	}
}

/*
 * Clean up
 */
void traceExit(void) {
	if(tracingEnabled) {
		traceSClean();
		if(treeEnabled)
			traceTClean();
	}
}

/*
 * Returns wether tracing is enabled
 */
Bool traceIsEnabled(void) {
	return tracingEnabled;
}

/**
 * Disables the tree-trace
 */
void traceDisableTree(void) {
	treeEnabled = false;
}

/*
 * Returns the current stack-level, that means the depth in our current call-hierarchie
 */
int traceGetStackLevel(void) {
	return traceSGetStackLevel(0);
}

/*
 * Handles the JAL and JALR instruction
 */
void traceHandleJAL(int instrCount,Word oldPC,Word newPC) {
	tTraceId id;
	unsigned int args[4];
	int i;

	if(!tracingEnabled) {
		return;
	}

	/* build arguments */
	for(i = 0; i < 4; i++) {
		args[i] = cpuGetReg(i + 4);
	}

	id = getTraceId();
	if(id != 0) {
		traceSAddNode(id,true,instrCount,oldPC,newPC,0,args);
		if(treeEnabled)
			traceTAddNode(id,true,instrCount,oldPC,newPC,0,args);
	}
	traceSAddNode(0,true,instrCount,oldPC,newPC,0,args);
	if(treeEnabled)
		traceTAddNode(0,true,instrCount,oldPC,newPC,0,args);
}

/*
 * Handles a JR
 */
int traceHandleJR(int instrCount,Word pc,int reg) {
	tTraceId id;
	if(!tracingEnabled) {
		return 0;
	}

	/* jr $31 is our return-instruction */
	if(reg == 31) {
		id = getTraceId();
		if(id != 0) {
			traceSRemoveNode(id,true,instrCount,pc);
			if(treeEnabled)
				traceTRemoveNode(id,true,instrCount,pc);
		}
		traceSRemoveNode(0,true,instrCount,pc);
		if(treeEnabled)
			traceTRemoveNode(0,true,instrCount,pc);
		return 1;
	}

	return 0;
}

/*
 * Handles a interrupt-service
 */
void traceHandleIS(int instrCount,Word oldPC,Word newPC,int irqnum) {
	tTraceId id;
	if(!tracingEnabled) {
		return;
	}

	id = getTraceId();
	if(id != 0) {
		traceSAddNode(id,false,instrCount,oldPC,newPC,irqnum,NULL);
		if(treeEnabled)
			traceTAddNode(id,false,instrCount,oldPC,newPC,irqnum,NULL);
	}
	traceSAddNode(0,false,instrCount,oldPC,newPC,irqnum,NULL);
	if(treeEnabled)
		traceTAddNode(0,false,instrCount,oldPC,newPC,irqnum,NULL);
}

/*
 * Handles a return-from-exception
 */
void traceHandleRFX(int instrCount,Word oldPC) {
	tTraceId id;
	if(!tracingEnabled) {
		return;
	}

	id = getTraceId();
	if(id != 0) {
		traceSRemoveNode(id,false,instrCount,oldPC);
		if(treeEnabled)
			traceTRemoveNode(id,false,instrCount,oldPC);
	}
	traceSRemoveNode(0,false,instrCount,oldPC);
	if(treeEnabled)
		traceTRemoveNode(0,false,instrCount,oldPC);
}

void doSimpleTrace(char *tokens[], int n) {
	tTraceId id;
	if(!tracingEnabled) {
		cPrintf("Sorry, tracing is disabled. Please specify a map-file via \"-m\"\n");
		return;
	}

	if(n < 2)
		id = readTraceId();
	else
		id = atoi(tokens[1]);
	if(id >= traceCount)
		cPrintf("Illegal trace!\n");
	else
		traceSPrint(id);
}

void doTreeTrace(char *tokens[], int n) {
	tTraceId id;
	if(!tracingEnabled) {
		cPrintf("Sorry, tracing is disabled. Please specify a map-file via \"-m\"\n");
		return;
	}
	if(!treeEnabled) {
		cPrintf("Sorry, tree-trace is disabled\n");
		return;
	}

	if(n < 2) {
		cPrintf("Usage: trf <file> [<trace>]!\n");
		return;
	}

	if(n < 3)
		id = readTraceId();
	else
		id = atoi(tokens[2]);

	if(id >= traceCount)
		cPrintf("Illegal trace!\n");
	else
		traceTPrint(id,tokens[1]);
}

static tTraceId readTraceId(void) {
	unsigned int i;
	TLB_Entry e = mmuGetTLB(0);
	cPrintf("Please choose one of the following traces:\n");
	for(i = 0; i < traceCount; i++) {
		if(i == 0)
			cPrintf("  %2d:     all   ",i);
		else {
			if(traces[i] == e.frame)
				cPrintf("  %2d: *%08x*",i,traces[i]);
			else
				cPrintf("  %2d:  %08x ",i,traces[i]);
		}
		if(i < traceCount - 1 && i % TRACES_PER_LINE == (TRACES_PER_LINE - 1))
			cPrintf("\n");
	}
	cPrintf("\nYour choice: ");
	scanf("%u",&i);
	return i < traceCount ? i : MAX_TRACES;
}

static tTraceId getTraceId(void) {
	int i;
	TLB_Entry e = mmuGetTLB(0);
	if(e.page < 0x80000000 || e.page >= 0xC0000000)
		return 0;
	for(i = 0; i < traceCount; i++) {
		if(e.frame == traces[i])
			return i;
	}

	if(traceCount == MAX_TRACES)
		error("Max. number of traces reached!");

	traces[traceCount] = e.frame;
	return traceCount++;
}
