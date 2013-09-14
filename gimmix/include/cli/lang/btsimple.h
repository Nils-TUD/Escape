/**
 * $Id$
 */

#ifndef BTSIMPLE_H_
#define BTSIMPLE_H_

#include "common.h"
#include "cli/lang/bt.h"

/* the node for our "simple-trace" */
typedef struct SNode tSNode;
struct SNode {
	sBacktraceData data;
	/* pointer to prev node. NULL if there is no prev */
	tSNode *prev;
	/* the pointer to the next node. NULL if there is no next */
	tSNode *next;
};

/*
 * Returns the current stack-level, that means the depth in our current call-hierarchie
 */
int btsimple_getStackLevel(tTraceId id);

/*
 * Inits the simple-trace
 */
void btsimple_init(void);

/*
 * Cleans up the given simple trace
 *
 * @param id the trace-id
 */
void btsimple_clean(tTraceId id);

/**
 * Cleans up all simple traces
 */
void btsimple_cleanAll(void);

/*
 * Adds a new node to the simple-trace
 */
void btsimple_add(tTraceId id,bool isFunc,octa instrCount,octa oldPC,octa newPC,octa threadId,
                  int ex,octa bits,size_t argCount,octa *args);

/*
 * Removes the last node from the simple-trace
 */
void btsimple_remove(tTraceId id,bool isFunc,octa instrCount,octa oldPC,octa threadId,
                     size_t argCount,octa *args);

/*
 * Prints the simple-trace
 */
void btsimple_print(tTraceId id);

#endif /*BTSIMPLE_H_*/
