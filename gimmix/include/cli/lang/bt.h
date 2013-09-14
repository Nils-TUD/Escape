/**
 * $Id$
 */

#ifndef BT_H_
#define BT_H_

#include "common.h"

#define MAX_TRACES		2048
#define MAX_ARGS		4

typedef unsigned int tTraceId;

typedef struct {
	/* wether it is a function or an interrupt */
	bool isFunc;
	/* the instruction-counter before the call / return */
	octa instrCount;
	/* program-counter before the call / return */
	octa oldPC;
	/* our new pc */
	octa newPC;
	/* the currently running thread */
	octa threadId;
	union {
		/* for interrupts */
		struct {
			int ex;
			octa bits;
		} intrpt;
		/* for function-calls */
		struct {
			size_t argCount;
			octa args[MAX_ARGS];
		} func;
	} d;
} sBacktraceData;

/*
 * Returns the current stack-level, that means the depth in our current call-hierarchie
 */
int bt_getStackLevel(void);

/**
 * Assignes the fields of <data> from the given arguments
 */
void bt_fillForAdd(sBacktraceData *data,bool isFunc,octa instrCount,
		octa oldPC,octa newPC,octa threadId,int ex,octa bits,size_t argCount,octa *args);

/**
 * Assignes the fields of <data> from the given arguments
 */
void bt_fillForRemove(sBacktraceData *data,bool isFunc,octa instrCount,octa oldPC,octa threadId,
                      size_t argCount,octa *args);

#endif /* BT_H_ */
