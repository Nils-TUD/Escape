#ifndef TRACEMAP_H_
#define TRACEMAP_H_

/*
 * one entry for the Function list.
 */
typedef struct tFN {
	char* name;
	unsigned int   address;
} traceFunctionNode;

/*
 * Get the Function name spezified by the given address. If there isn't a name, return "Unnamed"
 */
char *traceGetFuncName(Word address);

/*
 * Get the address of the given symbol; 0 if not found
 */
Word traceGetAddress(char *name);

/**
 * Searches for the nearest function to <address>
 */
const char *traceGetNearestFuncName(Word address,Word *nearest);

/*
 * Trace the Parse Map and Store the Functions into the traceFunctionNode
 */
int traceParseMap(char *mapName);

#endif /*TRACEMAP_H_*/
