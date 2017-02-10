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

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Mounts the device denoted by <fs> at <path> into given mountspace
 *
 * @param ms the mountspace to mount the fs into
 * @param fs the file-descriptor to the filesystem that should be mounted
 * @param path the path to mount at
 * @return 0 on success
 */
A_CHECKRET int mount(int ms,int fs,const char *path);

/**
 * Remounts <dir> at <path> with given permissions flags.
 *
 * @param ms the mountspace to mount the fs into
 * @param dir the file descriptor to the directory to remount
 * @param path the path to mount it at
 * @param perm the permissions (O_RDONLY | O_WRONLY | O_EXEC) to use; only downgrading is allowed
 * @return 0 on success
 */
A_CHECKRET int remount(int ms,int dir,const char *path,uint perm);

/**
 * Unmounts the device mounted at <path> from given mountspace
 *
 * @param ms the mountspace
 * @param path the path
 * @return 0 on success
 */
A_CHECKRET int unmount(int ms,const char *path);

/**
 * Clones the given mountspace into a new one with the given name and assigns it to the current
 * process.
 *
 * @param ms the mountspace to clone
 * @param name the name for the new mountspace
 * @return 0 on success
 */
static inline int clonems(int ms,const char *name) {
	return syscall2(SYSCALL_CLONEMS,ms,(ulong)name);
}

/**
 * Joins the mountspace denoted by <ms>.
 *
 * @param id the mountspace to join
 * @return 0 on success
 */
static inline int joinms(int ms) {
	return syscall1(SYSCALL_JOINMS,ms);
}

#if defined(__cplusplus)
}
#endif
