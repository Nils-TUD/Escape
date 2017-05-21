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
 * Each process has its own mountspace. It can be cloned and new mountspaces can be joined. The
 * mountspace defines the filesystem tree, that the user sees, given by mointpoints that make file
 * systems available at specific points in the tree. Mountpoints can also reduce permissions for
 * a subtree via remount.
 *
 * On Escape, symbolic links need special attention:
 * 1) if you have a symbolic link in a subtree (A), where you have less permissions than in the
 *    subtree the symbolic link points to (B), you might not be able to resolve the symlink because
 *    of missing permissions in A, making the permissions in B irrelevant. This is because the
 *    mountspace does not know about symbolic links and thus assumes that you stay in A.
 *    If you have sufficient permissions to resolve the symlink, the permissions of B apply.
 * 2) if you want to walk back from the destination of a symbolic link, you should resolve the
 *    symbolic link first and walk back in a second step. Otherwise, you might end up at the wrong
 *    place. This is a good idea anyway, because it makes it clear that you expect a symbol link.
 */

/**
 * Mounts the device denoted by <fs> at <path> into given mountspace
 *
 * @param ms the mountspace to mount the fs into
 * @param fs the file-descriptor to the filesystem that should be mounted
 * @param path the path to mount at (has to be canonical!)
 * @return 0 on success
 */
A_CHECKRET int mount(int ms,int fs,const char *path);

/**
 * Remounts <dir> with given permissions flags. Note that <dir> has to be opened with a canonical
 * path.
 *
 * @param ms the mountspace to mount the fs into
 * @param dir the file descriptor to the directory to remount
 * @param perm the permissions (O_RDONLY | O_WRONLY | O_EXEC) to use; only downgrading is allowed
 * @return 0 on success
 */
A_CHECKRET int remount(int ms,int dir,uint perm);

/**
 * Unmounts the device mounted at <path> from given mountspace
 *
 * @param ms the mountspace
 * @param path the path (has to be canonical!)
 * @return 0 on success
 */
A_CHECKRET int unmount(int ms,const char *path);

/**
 * Clones the current mountspace into a new one with the given name and assigns it to the current
 * process.
 *
 * @param name the name for the new mountspace
 * @return 0 on success
 */
static inline int clonems(const char *name) {
	return syscall1(SYSCALL_CLONEMS,(ulong)name);
}

/**
 * Joins the mountspace denoted by <ms>. This is only allowed for the root user.
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
