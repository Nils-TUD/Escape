/**
 * $Id: mem.h 114 2011-02-07 12:31:28Z nasmussen $
 */

#ifndef MEM_H_
#define MEM_H_

/**
 * This module builds a layer upon malloc and so on. If the allocation fails, it prints an
 * error-message and stops the program.
 */

#include <stddef.h>

/**
 * Calls malloc()
 */
void *mem_alloc(size_t n);

/**
 * Calls calloc()
 */
void *mem_calloc(size_t n,size_t size);

/**
 * Calls realloc()
 */
void *mem_realloc(void *p,size_t n);

/**
 * Calls free()
 */
void mem_free(void *p);

#endif /* MEM_H_ */
