/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MEM_H_
#define MEM_H_

#include <esc/common.h>

/* description of a binary */
typedef struct {
	inode_t ino;
	dev_t dev;
	time_t modifytime;
} sBinDesc;

/* the region-types */
#define REG_TEXT			0
#define REG_RODATA			1
#define REG_DATA			2
#define REG_STACK			3
#define REG_STACKUP			4
#define REG_SHM				5
#define REG_DEVICE			6
#define REG_TLS				7
#define REG_SHLIBTEXT		8
#define REG_SHLIBDATA		9
#define REG_DLDATA			10
#define REG_PHYS			11

/* protection-flags */
#define PROT_READ			1
#define PROT_WRITE			2

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Changes the size of the process's data-area. If <count> is positive <count> pages
 * will be added to the end of the data-area. Otherwise <count> pages will be removed at the
 * end.
 *
 * @param count the number of pages to add / remove
 * @return the *old* end of the data-segment. NULL indicates an error
 */
void *chgsize(ssize_t count) A_CHECKRET;

/**
 * Adds a region to the current process at the appropriate virtual address (depending on
 * existing regions and the region-type) from given binary.
 *
 * @param bin the binary (may be NULL; means: no demand-loading possible)
 * @param binOffset the offset of the region in the binary (for demand-loading)
 * @param byteCount the number of bytes of the region
 * @param loadCount number of bytes to load from disk (the rest is zero'ed)
 * @param type the region-type (see REG_*)
 * @param virt the virtual address (required for text, rodata and data, otherwise 0)
 * @return the address of the region on success, NULL on failure
 */
void *regadd(sBinDesc *bin,uintptr_t binOffset,size_t byteCount,size_t loadCount,uint type,
             uintptr_t virt);

/**
 * Changes the protection of the region denoted by the given address.
 *
 * @param addr the virtual address
 * @param prot the new protection-setting (PROT_*)
 * @return 0 on success
 */
int regctrl(uintptr_t addr,uint prot);

/**
 * Maps <count> bytes at <phys> into the virtual user-space and returns the start-address.
 *
 * @param phys the physical start-address to map
 * @param count the number of bytes to map
 * @return the virtual address where it has been mapped or NULL if an error occurred
 */
void *mapphys(uintptr_t phys,size_t count) A_CHECKRET;

/**
 * Maps the multiboot module with given name in the virtual address space and returns the
 * start-address.
 *
 * @param name the module-name
 * @param size will be set to the module size
 * @return the virtual address where it has been mapped or NULL if an error occurred
 */
void *mapmod(const char *name,size_t *size) A_CHECKRET;

/**
 * Allocates <count> bytes contiguous physical memory, <align>-bytes aligned.
 *
 * @param phys will be set to the chosen physical address
 * @param count the byte-count
 * @param aligh the alignment (in bytes)
 * @return the virtual address where it has been mapped or NULL if an error occurred
 */
void *allocphys(uintptr_t *phys,size_t count,size_t align) A_CHECKRET;

/**
 * Creates a shared-memory region
 *
 * @param name the name
 * @param byteCount the number of bytes
 * @return the address on success or NULL
 */
void *shmcrt(const char *name,size_t byteCount) A_CHECKRET;

/**
 * Joines a shared-memory region
 *
 * @param name the name
 * @return the address on success or NULL
 */
void *shmjoin(const char *name) A_CHECKRET;

/**
 * Leaves a shared-memory region
 *
 * @param name the name
 * @return 0 on success
 */
int shmleave(const char *name);

/**
 * Deletes a shared-memory region
 *
 * @param name the name
 * @return 0 on success
 */
int shmdel(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* MEM_H_ */
