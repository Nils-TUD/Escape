#ifndef TRACES_H_
#define TRACES_H_

#include "../common.h"
#include "trace.h"

/* the node for our "simple-trace" */
typedef struct SNode tSNode;
struct SNode {
	/* wether it is a function or an interrupt */
	Bool isFunc;
	/* the instruction-counter before the call / return */
	int instrCount;
	/* program-counter before the call */
	Word oldPC;
	/* our new pc */
	Word newPC;
	union {
		/* for interrupts */
		struct {
			int irq;
		} intrpt;
		/* for function-calls */
		struct {
			/* we want to save 4 arguments */
			unsigned int args[4];
		} func;
	};
	/* pointer to prev node. NULL if there is no prev */
	tSNode *prev;
	/* the pointer to the next node. NULL if there is no next */
	tSNode *next;
};

/*
 * Returns the current stack-level, that means the depth in our current call-hierarchie
 */
int traceSGetStackLevel(tTraceId id);

/*
 * Inits the simple-trace
 */
void traceSInit(void);

/*
 * Cleans up the simple trace
 */
void traceSClean(void);

/*
 * Adds a new node to the simple-trace
 */
void traceSAddNode(tTraceId id,Bool isFunc,int instrCount,Word oldPC,Word newPC,int irq,
		unsigned int *args);

/*
 * Removes the last node from the simple-trace
 */
void traceSRemoveNode(tTraceId id,Bool isFunc,int instrCount,Word oldPC);

/*
 * Prints the simple-trace
 */
void traceSPrint(tTraceId id);

#endif /*TRACES_H_*/
