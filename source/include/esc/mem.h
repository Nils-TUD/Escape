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
#include <sys/mem/region.h>

/* protection-flags */
#define PROT_READ			0
#define PROT_WRITE			RF_WRITABLE
#define PROT_EXEC			RF_EXECUTABLE

/* mapping flags */
#define MAP_SHARED			RF_SHAREABLE
#define MAP_PRIVATE			0
#define MAP_STACK			RF_STACK
#define MAP_GROWABLE		RF_GROWABLE
#define MAP_GROWSDOWN		RF_GROWS_DOWN
#define MAP_NOFREE			RF_NOFREE
#define MAP_TLS				RF_TLS
#define MAP_POPULATE		256UL
#define MAP_NOMAP			512UL
#define MAP_FIXED			1024UL

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
 * Creates a new mapping in the virtual address space of the calling process. This might be anonymous
 * memory or a file, denoted by <fd>.
 *
 * @param addr the desired address (might be ignored)
 * @param length the number of bytes
 * @param loadLength the number of bytes that should be loaded from the given file (the rest is zero'd)
 * @param prot the protection flags (PROT_*)
 * @param flags the mapping flags (MAP_*)
 * @param fd optionally, a file descriptor of the file to map
 * @param offset the offset in the file
 * @return the virtual address or NULL if the mapping failed
 */
void *mmap(void *addr,size_t length,size_t loadLength,int prot,int flags,int fd,off_t offset);

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
int mprotect(void *addr,uint prot);

/**
 * Unmaps the region denoted by <addr>
 *
 * @param addr the address of the region
 * @return 0 on success
 */
int munmap(void *addr);

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
