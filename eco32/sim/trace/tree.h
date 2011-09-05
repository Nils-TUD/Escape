#ifndef TRACET_H_
#define TRACET_H_

#include "../common.h"
#include "trace.h"

/* the node for our "tree-trace" */
typedef struct TNode tTNode;
struct TNode {
	/* wether it is a function or an interrupt */
	Bool isFunc;
	/* enter or leave? */
	Bool isEnter;
	/* the instruction-counter before the call / return */
	int instrCount;
	/* program-counter before the call / return */
	Word oldPC;
	/* our new pc */
	Word newPC;
	/* processor status word */
	Word psw;
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
	tTNode *prev;
	/* the pointer to the next node. NULL if there is no next */
	tTNode *next;
};

/*
 * Inits the tree-trace
 */
void traceTInit(void);

/*
 * Cleans up the tree trace
 */
void traceTClean(void);

/*
 * Adds a new node to the tree-trace
 */
void traceTAddNode(tTraceId id,Bool isFunc,int instrCount,Word oldPC,Word newPC,int irq,
		unsigned int *args);

/*
 * Removes the last node from the tree-trace
 */
void traceTRemoveNode(tTraceId id,Bool isFunc,int instrCount,Word oldPC);

/*
 * Prints the tree-trace into the given file
 */
void traceTPrint(tTraceId id,char* filename);

#endif /*TRACET_H_*/
