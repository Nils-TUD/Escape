/**
 * $Id$
 */

#ifndef ECO32_PMEM_H_
#define ECO32_PMEM_H_

/**
 * Physical memory layout:
 * 0x00000000: +-----------------------------------+   -----
 *             |                                   |     |
 *             |          kernel code+data         |     |
 *             |                                   |
 *             +-----------------------------------+  unmanaged
 *             |         multiboot-modules         |
 *             +-----------------------------------+     |
 *             |              MM-stack             |     |
 *             +-----------------------------------+   -----
 *             |             MM-bitmap             |     |
 *             +-----------------------------------+     |
 *             |                                   |
 *             |                                   |  bitmap managed (2 MiB)
 *             |                ...                |
 *             |                                   |     |
 *             |                                   |     |
 *             +-----------------------------------+   -----
 *             |                                   |     |
 *             |                                   |
 *             |                ...                |  stack managed (remaining)
 *             |                                   |
 *             |                                   |     |
 * 0xFFFFFFFF: +-----------------------------------+   -----
 */

#define PAGE_SIZE				(4 * K)
#define PAGE_SIZE_SHIFT			12

#endif /* ECO32_PMEM_H_ */
