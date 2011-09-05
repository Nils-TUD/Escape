/**
 * $Id: mem.c 114 2011-02-07 12:31:28Z nasmussen $
 */

#include <stddef.h>
#include <stdlib.h>
#include "mmix/mem.h"
#include "mmix/error.h"

void *mem_alloc(size_t n) {
	void *p = malloc(n);
	if(!p)
		error("Not enough memory to allocate %u bytes",n);
	return p;
}

void *mem_calloc(size_t n,size_t size) {
	void *p = calloc(n,size);
	if(!p)
		error("Not enough memory to allocate %u bytes",n);
	return p;
}

void *mem_realloc(void *p,size_t n) {
	p = realloc(p,n);
	if(!p)
		error("Not enough memory to allocate %u bytes",n);
	return p;
}

void mem_free(void *p) {
	free(p);
}
