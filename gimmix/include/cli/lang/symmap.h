#ifndef TRACEMAP_H_
#define TRACEMAP_H_

#include "common.h"

/* max 4 maps supported for now */
#define MAX_MAPS		4

/**
 * Get the function that is near the given address.
 */
const char *symmap_getNearestFuncName(octa address,octa *nearest);

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
