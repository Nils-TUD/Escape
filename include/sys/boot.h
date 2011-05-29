/**
 * $Id$
 */

#ifndef BOOT_H_
#define BOOT_H_

#include <esc/common.h>

#ifdef __i386__
#include <sys/arch/i586/boot.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/boot.h>
#endif

/**
 * @return size of the kernel (in bytes)
 */
size_t boot_getKernelSize(void);

/**
 * @return size of the multiboot-modules (in bytes)
 */
size_t boot_getModuleSize(void);

/**
 * @return the usable memory in bytes
 */
size_t boot_getUsableMemCount(void);

#if DEBUGGING

/**
 * Prints all interesting elements of the multi-boot-structure
 */
void boot_dbg_print(void);

#endif

#endif /* BOOT_H_ */
