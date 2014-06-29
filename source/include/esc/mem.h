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

#include <esc/common.h>
#include <esc/syscalls.h>

/* protection-flags */
#define PROT_READ			0
#define PROT_WRITE			2048UL
#define PROT_EXEC			4096UL

/* mapping flags */
#define MAP_PRIVATE			0		/* make the region non-sharable */
#define MAP_SHARED			1UL		/* make the region shareable */
#define MAP_GROWABLE		2UL		/* make the region growable (e.g. heap, stack) */
#define MAP_GROWSDOWN		4UL		/* for growable regions: let them grow downwards */
#define MAP_STACK			8UL		/* for stack regions */
#define MAP_TLS				16UL	/* for TLS regions */
#define MAP_LOCKED			32UL	/* lock the region in memory, i.e. don't swap it out */
#define MAP_POPULATE		64UL	/* fault-in all pages at the beginning */
#define MAP_NOSWAP			128UL	/* if not enough memory for the mapping, don't swap but fail */
#define MAP_FIXED			256UL	/* put the region exactly at the given address */

#define MAP_PHYS_ALLOC		0		/* allocate physical memory */
#define MAP_PHYS_MAP		1		/* map the specified physical memory */

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

/**
 * Creates a file in /sys/proc/<pid>/shm/ with a unique name and <oflag> as flags for create.
 * The file is intended to be mapped with mmap().
 *
 * @param oflag the open flags
 * @param mode the mode to set
 * @param name will be set to the name
 * @return the file descriptor on success
 */
int pshm_create(int oflag,mode_t mode,ulong *name);

/**
 * Unlinks the file previously created by pshm_create.
 *
 * @parma name the name of the shm
 */
int pshm_unlink(ulong name);

/**
 * Creates/opens a file in /sys/shm/ with given name and opens it with <oflag>. If <oflag> contains
 * IO_CREATE, <mode> is used for the permissions. This file is intended to be mapped with mmap().
 *
 * @param name the filename
 * @param oflag the open flags
 * @param mode the mode to set
 * @return the file descriptor on success
 */
int shm_open(const char *name,int oflag,mode_t mode);

/**
 * Renames the given shared-memory file from <old> to <newName>.
 *
 * @param old the old name
 * @param newName the new name
 * @return 0 on success
 */
int shm_rename(const char *old,const char *newName);

/**
 * Unlinks the file previously created by shm_open.
 *
 * @parma name the filename
 */
int shm_unlink(const char *name);

#if defined(__cplusplus)
}
#endif
