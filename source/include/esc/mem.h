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

#pragma once

#include <esc/common.h>

/* description of a binary */
typedef struct {
	inode_t ino;
	dev_t dev;
	time_t modifytime;
	char filename[24];
} sBinDesc;

/* the region-types */
#define REG_TEXT			0	/* text-region of the program; only for dynamic linker */
#define REG_RODATA			1	/* rodata-region of the program; only for dynamic linker */
#define REG_DATA			2	/* data-region of the program; only for dynamic linker */
#define REG_TLS				7	/* TLS of the current thread; only for dynamic linker */
#define REG_SHLIBTEXT		8	/* shared library text; only for dynamic linker */
#define REG_SHLIBDATA		9	/* shared library data; only for dynamic linker */

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
 * @param offset the offset of the region in the binary (for demand-loading)
 * @param byteCount the number of bytes of the region
 * @param loadCount number of bytes to load from disk (the rest is zero'ed)
 * @param type the region-type (see REG_*)
 * @param virt the virtual address (required for text, rodata and data, otherwise 0)
 * @return the address of the region on success, NULL on failure
 */
void *regadd(sBinDesc *bin,uintptr_t offset,size_t byteCount,size_t loadCount,uint type,
             uintptr_t virt);

/**
 * Maps <count> bytes at *<phys> into the virtual user-space and returns the start-address.
 *
 * @param phys will be set to the chosen physical address; if *phys != 0, this address will
 *   be used.
 * @param count the number of bytes to map
 * @param aligh the alignment (in bytes); if not 0, contiguous physical memory, aligned by <align>
 *   will be used.
 * @return the virtual address where it has been mapped or NULL if an error occurred
 */
void *regaddphys(uintptr_t *phys,size_t count,size_t align) A_CHECKRET;

/**
 * Maps the multiboot module with given name in the virtual address space and returns the
 * start-address.
 *
 * @param name the module-name
 * @param size will be set to the module size
 * @return the virtual address where it has been mapped or NULL if an error occurred
 */
void *regaddmod(const char *name,size_t *size) A_CHECKRET;

/**
 * Changes the protection of the region denoted by the given address.
 *
 * @param addr the virtual address
 * @param prot the new protection-setting (PROT_*)
 * @return 0 on success
 */
int regctrl(void *addr,uint prot);

/**
 * Removes the region denoted by the virtual address of the region, <addr>, from the address
 * space of the current process.
 *
 * @param addr the address of the region
 * @return 0 on success
 */
int regrem(void *addr);

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
