/**
 * $Id$
 */

#ifndef I586_PMEM_H_
#define I586_PMEM_H_

/**
 * Physical memory layout:
 * 0x00000000: +-----------------------------------+   -----
 *             |                                   |     |
 *             |                VM86               |     |
 *             |                                   |     |
 * 0x00100000: +-----------------------------------+     |
 *             |                                   |
 *             |          kernel code+data         |  unmanaged
 *             |                                   |
 *             +-----------------------------------+     |
 *             |         multiboot-modules         |     |
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

/* the physical start-address of the kernel-area */
#define KERNEL_AREA_P_ADDR		0x0
/* the physical start-address of the kernel itself */
#define KERNEL_P_ADDR			(1 * M)

#define PAGE_SIZE				(4 * K)
#define PAGE_SIZE_SHIFT			12

#endif /* I586_PMEM_H_ */
