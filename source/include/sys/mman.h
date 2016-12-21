/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <sys/common.h>
#include <sys/syscalls.h>

/* protection-flags */
enum {
	PROT_READ			= 0,
	PROT_WRITE			= 1024UL,
	PROT_EXEC			= 2048UL,
};

/* mapping flags */
enum {
	MAP_PRIVATE			= 0,		/* make the region non-sharable */
	MAP_SHARED			= 1UL,		/* make the region shareable */
	MAP_GROWABLE		= 2UL,		/* make the region growable (e.g. heap, stack) */
	MAP_GROWSDOWN		= 4UL,		/* for growable regions: let them grow downwards */
	MAP_STACK			= 8UL,		/* for stack regions */
	MAP_LOCKED			= 16UL,		/* lock the region in memory, i.e. don't swap it out */
	MAP_POPULATE		= 32UL,		/* fault-in all pages at the beginning */
	MAP_NOSWAP			= 64UL,		/* if not enough memory for the mapping, don't swap but fail */
	MAP_FIXED			= 128UL,	/* put the region exactly at the given address */

	MAP_PHYS_ALLOC		= 0,		/* allocate physical memory */
	MAP_PHYS_MAP		= 1,		/* map the specified physical memory */
};

enum {
	MATTR_WC			= 1,		/* write combining */
};

struct mmap_params {
	void *addr;
	size_t length;
	size_t loadLength;
	int prot;
	int flags;
	int fd;
	off_t offset;
};

#if defined(__cplusplus)
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
void *mmapphys(uintptr_t *phys,size_t count,size_t align,int flags) A_CHECKRET;

/**
 * Sets the given attributes to the physical memory range <phys> .. <phys>+<bytes>.
 * <bytes> needs to be page-aligned and <phys> needs to be <bytes>-aligned.
 *
 * @param phys the physical address
 * @param bytes the number of bytes
 * @param attr the attributes (MATTR_*)
 * @return 0 on success
 */
static inline int mattr(uintptr_t phys,size_t bytes,int attr) {
	return syscall3(SYSCALL_MATTR,phys,bytes,attr);
}

/**
 * Changes the protection of the region denoted by the given address.
 *
 * @param addr the virtual address
 * @param prot the new protection-setting (PROT_*)
 * @return 0 on success
 */
static inline int mprotect(void *addr,uint prot) {
	return syscall2(SYSCALL_MPROTECT,(ulong)addr,prot);
}

/**
 * Unmaps the region denoted by <addr>
 *
 * @param addr the address of the region
 * @return 0 on success
 */
static inline int munmap(void *addr) {
	return syscall1(SYSCALL_MUNMAP,(ulong)addr);
}

/**
 * Locks the region denoted by <addr> and makes sure that all pages of that region are in memory..
 *
 * @param addr the address of the region
 * @param flags only MAP_NOSWAP is supported atm
 * @return 0 on success
 */
static inline int mlock(void *addr,int flags) {
	return syscall2(SYSCALL_MLOCK,(ulong)addr,flags);
}

/**
 * Locks all regions currently in the address space and makes sure that all pages are in memory.
 * Note that this will not start swapping, if necessary, but report an error if there is not enough
 * memory.
 *
 * @return 0 on success
 */
static inline int mlockall(void) {
	return syscall0(SYSCALL_MLOCKALL);
}

#if defined(__cplusplus)
}
#endif
