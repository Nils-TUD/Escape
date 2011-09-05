#ifndef TRACE_H_
#define TRACE_H_

#include "../common.h"

/* max 4 maps supported for now */
#define MAX_MAPS		4

#define MAX_TRACES		256

typedef unsigned int tTraceId;

/* Tracing may be disabled. This makes sense for example if it would consume too much
 * memory */
extern Bool tracingEnabled;

/*
 * Inits the trace-mechanism
 */
void traceInit(char* mapName[]);

/*
 * Resets the trace-mechanism
 */
void traceReset(void);

/*
 * Returns wether tracing is enabled
 */
Bool traceIsEnabled(void);

/**
 * Disables the tree-trace
 */
void traceDisableTree(void);

/*
 * Clean up
 */
void traceExit(void);

/*
 * Returns the current stack-level, that means the depth in our current call-hierarchie
 */
int traceGetStackLevel(void);

/*
 * Handles the JAL and JALR instruction
 */
void traceHandleJAL(int instrCount,Word oldPC,Word newPC);

/*
 * Handles a JR. Returns wether it was a "real return"
 */
int traceHandleJR(int instrCount,Word pc,int reg);

/*
 * Handles a interrupt-service
 */
void traceHandleIS(int instrCount,Word oldPC,Word newPC,int irqnum);

/*
 * Handles a return-from-exception
 */
void traceHandleRFX(int instrCount,Word oldPC);

/*
 * Print the current stack-trace
 */
void doSimpleTrace(char *tokens[], int n);

/**
 * Print the complete trace to file
 */
void doTreeTrace(char *tokens[], int n);

#endif /*TRACE_H_*/
