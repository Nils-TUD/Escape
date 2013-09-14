#ifndef TRACET_H_
#define TRACET_H_

#include "common.h"
#include "cli/lang/bt.h"

/* the node for our "tree-trace" */
typedef struct TNode tTNode;
struct TNode {
	sBacktraceData data;
	/* enter or leave? */
	bool isEnter;
	/* pointer to prev node. NULL if there is no prev */
	tTNode *prev;
	/* the pointer to the next node. NULL if there is no next */
	tTNode *next;
};

/*
 * Inits the tree-trace
 */
void bttree_init(void);

/*
 * Cleans up the given tree trace
 *
 * @param id the trace-id
 */
void bttree_clean(tTraceId id);

/**
 * Cleans up all tree traces
 */
void bttree_cleanAll(void);

/*
 * Adds a new node to the tree-trace
 */
void bttree_add(tTraceId id,bool isFunc,octa instrCount,octa oldPC,octa newPC,octa threadId,
		int ex,octa bits,size_t argCount,octa *args);

/*
 * Removes the last node from the tree-trace
 */
void bttree_remove(tTraceId id,bool isFunc,octa instrCount,octa oldPC,octa threadId,
                   size_t argCount,octa *args);

/*
 * Prints the tree-trace into the given file
 */
void bttree_print(tTraceId id,const char *filename);

#endif /*TRACET_H_*/
