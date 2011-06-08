/**
 * $Id$
 */

#include <esc/common.h>
#include <sys/arch/mmix/mem/addrspace.h>
#include <assert.h>

/*
 * the basic idea here is to give each process a different address-space (means: the number in rV
 * and all PTEs and PTPs), as long it is possible. As soon as 1024 processes are running, the next
 * one has to reuse an existing address-space. This implies, that we have to clear the TCs whenever
 * we switch to a thread of the processes that have an address-space that is used by more than one
 * process. I think its better to spread the processes over the address-spaces instead of putting
 * all additional ones into one address-space. Because if two processes have a similar usage-count,
 * the number of useless TC-clears should be less than if two processes with a different usage-count
 * are put together into one address-space. So, the ideal system would be to put processes with a
 * similar usage-count together, but its not that easy to figure that out in advance. Therefore,
 * we spread the processes over the address-spaces and hope for the best :)
 */

/* rV.n has 10 bits */
#define ADDR_SPACE_COUNT		1024

static sAddressSpace addrSpaces[ADDR_SPACE_COUNT];
static sAddressSpace *freeList = NULL;
static sAddressSpace *usedList = NULL;
static sAddressSpace *lastUsed = NULL;

void aspace_init(void) {
	size_t i;
	freeList = addrSpaces;
	freeList->next = NULL;
	for(i = 1; i < ADDR_SPACE_COUNT; i++) {
		addrSpaces[i].no = i;
		addrSpaces[i].next = freeList;
		freeList = addrSpaces + i;
	}
}

sAddressSpace *aspace_alloc(void) {
	sAddressSpace *as = NULL;
	if(freeList != NULL) {
		/* remove the first of the freelist */
		as = freeList;
		freeList = freeList->next;
	}
	else {
		as = usedList;
		assert(as != NULL);
		/* remove from used-list */
		usedList = usedList->next;
	}

	/* append to used-list */
	as->next = NULL;
	if(lastUsed)
		lastUsed->next = as;
	else
		usedList = as;
	lastUsed = as;
	as->refCount++;
	return as;
}

void aspace_free(sAddressSpace *aspace) {
	if(--aspace->refCount == 0) {
		/* remove from usedlist */
		sAddressSpace *p = NULL,*as = usedList;
		while(as != NULL) {
			if(as == aspace) {
				if(as == lastUsed)
					lastUsed = p;
				if(p)
					p->next = as->next;
				else
					usedList = as->next;
				break;
			}
			p = as;
			as = as->next;
		}

		/* put on freelist */
		aspace->next = freeList;
		freeList = aspace;
	}
}
