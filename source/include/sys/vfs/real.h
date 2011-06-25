/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#ifndef VFSREAL_H_
#define VFSREAL_H_

#include <sys/common.h>
#include <esc/fsinterface.h>

/**
 * Inits vfs-real
 */
void vfs_real_init(void);

/**
 * Removes the given process (closes fs-communication-file)
 *
 * @param pid the process-id
 */
void vfs_real_removeProc(pid_t pid);

/**
 * Opens the given path with given flags for given process
 *
 * @param pid the process-id
 * @param flags read / write
 * @param path the path
 * @return 0 on success or the error-code
 */
file_t vfs_real_openPath(pid_t pid,uint flags,const char *path);

/**
 * Opens the given inode+devno with given flags for given process
 *
 * @param pid the process-id
 * @param flags read / write
 * @param ino the inode-number
 * @param dev the dev-number
 * @return 0 on success or the error-code
 */
file_t vfs_real_openInode(pid_t pid,uint flags,inode_t ino,dev_t dev);

/**
 * Retrieves information about the given (real!) path
 *
 * @param pid the process-id
 * @param path the path in the real filesystem
 * @param info should be filled
 * @return 0 on success
 */
int vfs_real_stat(pid_t pid,const char *path,sFileInfo *info);

/**
 * Retrieves information about the given inode on given device
 *
 * @param pid the process-id
 * @param ino the inode-number
 * @param devNo the device-number
 * @param info should be filled
 * @return 0 on success
 */
int vfs_real_istat(pid_t pid,inode_t ino,dev_t devNo,sFileInfo *info);

/**
 * Reads from the given inode at <offset> <count> bytes into the given buffer
 *
 * @param pid the process-id
 * @param inodeNo the inode
 * @param devNo the device-number
 * @param buffer the buffer to fill
 * @param offset the offset in the data
 * @param count the number of bytes to copy
 * @return the number of read bytes
 */
ssize_t vfs_real_read(pid_t pid,inode_t inodeNo,dev_t devNo,void *buffer,
		off_t offset,size_t count);

/**
 * Writes to the given inode at <offset> <count> bytes from the given buffer
 *
 * @param pid the process-id
 * @param inodeNo the inode
 * @param devNo the device-number
 * @param buffer the buffer
 * @param offset the offset in the inode
 * @param count the number of bytes to copy
 * @return the number of written bytes
 */
ssize_t vfs_real_write(pid_t pid,inode_t inodeNo,dev_t devNo,const void *buffer,
		off_t offset,size_t count);

/**
 * Creates a hardlink at <newPath> which points to <oldPath>
 *
 * @param pid the process-id
 * @param oldPath the link-target
 * @param newPath the link-path
 * @return 0 on success
 */
int vfs_real_link(pid_t pid,const char *oldPath,const char *newPath);

/**
 * Unlinks the given path. That means, the directory-entry will be removed and if there are no
 * more references to the inode, it will be removed.
 *
 * @param pid the process-id
 * @param path the path
 * @return 0 on success
 */
int vfs_real_unlink(pid_t pid,const char *path);

/**
 * Creates the given directory. Expects that all except the last path-component exist.
 *
 * @param pid the process-id
 * @param path the path
 * @return 0 on success
 */
int vfs_real_mkdir(pid_t pid,const char *path);

/**
 * Removes the given directory. Expects that the directory is empty (except '.' and '..')
 *
 * @param pid the process-id
 * @param path the path
 * @return 0 on success
 */
int vfs_real_rmdir(pid_t pid,const char *path);

/**
 * Mounts <device> at <path> with fs <type>
 *
 * @param pid the process-id
 * @param device the device-path
 * @param path the path to mount at
 * @param type the fs-type
 * @return 0 on success
 */
int vfs_real_mount(pid_t pid,const char *device,const char *path,uint type);

/**
 * Unmounts the device mounted at <path>
 *
 * @param pid the process-id
 * @param path the path
 * @return 0 on success
 */
int vfs_real_unmount(pid_t pid,const char *path);

/**
 * Writes all dirty objects of the filesystem to disk
 *
 * @param pid the process-id
 * @return 0 on success
 */
int vfs_real_sync(pid_t pid);

/**
 * Closes the given inode
 *
 * @param pid the process-id
 * @param inodeNo the inode
 * @param devNo the device-number
 */
void vfs_real_close(pid_t pid,inode_t inodeNo,dev_t devNo);

#endif /* VFSREAL_H_ */
