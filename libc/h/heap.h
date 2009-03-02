/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef HEAP_H_
#define HEAP_H_

/* an area in memory */
typedef struct sPubArea sPubArea;
struct sPubArea {
	const u32 size;
	void *const address;
	sPubArea *const next;
};

/* the number of entries in the occupied map */
#define OCC_MAP_SIZE	64

/**
 * @return the usable-list
 */
sPubArea *getUsableList(void);

/**
 * @return the occupied-list
 */
sPubArea **getOccupiedMap(void);

/**
 * @return the free-list
 */
sPubArea *getFreeList(void);

/**
 * Allocates <size> bytes on the heap and returns the pointer to the beginning of
 * the allocated memory. If there is not enough memory the function returns NULL.
 *
 * @param size the number of bytes to allocate
 * @return the address of the memory or NULL
 */
void *malloc(u32 size);

/**
 * Reallocates the area at given address to the given size. That means either your data will
 * be copied to a different address or your area will be resized.
 *
 * @param addr the address of your area
 * @param size the number of bytes your area should be resized to
 * @return the address (may be different) of your area or NULL if there is not enough mem
 */
void *realloc(void *addr,u32 size);

/**
 * Frees the area starting at <addr>.
 */
void free(void *addr);

/**
 * Prints the heap
 */
void printHeap(void);

#endif /* HEAP_H_ */
