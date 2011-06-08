/**
 * $Id$
 */

#ifndef ADDRSPACE_H_
#define ADDRSPACE_H_

#include <esc/common.h>

typedef struct sAddressSpace {
	ulong no;
	ulong refCount;
	struct sAddressSpace *next;
} sAddressSpace;

void aspace_init(void);
sAddressSpace *aspace_alloc(void);
void aspace_free(sAddressSpace *aspace);

#endif /* ADDRSPACE_H_ */
