#ifndef TRACEMAP_H_
#define TRACEMAP_H_

#include "common.h"

/* max 4 maps supported for now */
#define MAX_MAPS		4

/**
 * one entry for the Function list.
 */
typedef struct tFN {
	const char *name;
	octa address;
	struct tFN *next;
	struct tFN *prev;
} traceFunctionNode;

/**
 * Append an Entry to the traceFunctionList
 */
void symmap_append(traceFunctionNode* node);

/**
 * Get the Function name spezified by the given address. If there isn't a name, return "Unnamed"
 */
const char *symmap_getFuncName(octa address);

/**
 * Get the address of the given symbol; 0 if not found
 */
octa symmap_getAddress(const char *name);

/**
 * Trace the Parse Map and Store the Functions into the traceFunctionNode
 */
bool symmap_parse(const char *mapName);

#endif /*TRACEMAP_H_*/
