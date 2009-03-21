/**
 * @version		$Id: kheap.h 36 2008-11-09 16:57:55Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef KHEAP_H_
#define KHEAP_H_

#include "../h/common.h"

/**
 * Note that this function is intended for debugging-purposes only!
 *
 * @return the number of free bytes
 */
u32 kheap_getFreeMem(void);

/**
 * @param addr the area-address
 * @return the size of the area at given address (0 if not found)
 */
u32 kheap_getAreaSize(void *addr);

/**
 * Allocates <size> bytes in kernel-space and returns the pointer to the beginning of
 * the allocated memory. If there is not enough memory the function returns NULL.
 *
 * @param size the number of bytes to allocate
 * @return the address of the memory or NULL
 */
void *kheap_alloc(u32 size);

/**
 * Allocates space for <num> elements, each <size> big, on the heap and memset's the area to 0.
 * If there is not enough memory the function returns NULL.
 *
 * @param num the number of elements
 * @param size the size of each element
 * @return the address of the memory or NULL
 */
void *kheap_calloc(u32 num,u32 size);

/**
 * Reallocates the area at given address to the given size. That means either your data will
 * be copied to a different address or your area will be resized.
 *
 * @param addr the address of your area
 * @param size the number of bytes your area should be resized to
 * @return the address (may be different) of your area or NULL if there is not enough mem
 */
void *kheap_realloc(void *addr,u32 size);

/**
 * Frees the via kmalloc() allocated area starting at <addr>.
 */
void kheap_free(void *addr);

#if DEBUGGING

/**
 * Prints the kernel-heap data-structure
 */
void kheap_dbg_print(void);

#endif

#endif /* KHEAP_H_ */
