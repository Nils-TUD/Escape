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

#include "common.h"
#include <fsinterface.h>

/**
 * Inits vfs-real
 */
void vfsr_init(void);

/**
 * Opens the given path with given flags for given thread
 *
 * @param tid the thread-id
 * @param flags read / write
 * @param path the path
 * @return 0 on success or the error-code
 */
s32 vfsr_openFile(tTid tid,u8 flags,const char *path);

/**
 * Retrieves information about the given (real!) path
 *
 * @param tid the thread-id
 * @param path the path in the real filesystem
 * @param info should be filled
 * @return 0 on success
 */
s32 vfsr_getFileInfo(tTid tid,const char *path,sFileInfo *info);

/**
 * Reads from the given inode at <offset> <count> bytes into the given buffer
 *
 * @param tid the thread-id
 * @param file the 'real' file
 * @param inodeNo the inode
 * @param buffer the buffer to fill
 * @param offset the offset in the data
 * @param count the number of bytes to copy
 * @return the number of read bytes
 */
s32 vfsr_readFile(tTid tid,tFileNo file,tInodeNo inodeNo,u8 *buffer,u32 offset,u32 count);

/**
 * Writes to the given inode at <offset> <count> bytes from the given buffer
 *
 * @param tid the thread-id
 * @param file the 'real' file
 * @param inodeNo the inode
 * @param buffer the buffer
 * @param offset the offset in the inode
 * @param count the number of bytes to copy
 * @return the number of written bytes
 */
s32 vfsr_writeFile(tTid tid,tFileNo file,tInodeNo inodeNo,const u8 *buffer,u32 offset,u32 count);

/**
 * Creates a hardlink at <newPath> which points to <oldPath>
 *
 * @param tid the thread-id
 * @param oldPath the link-target
 * @param newPath the link-path
 * @return 0 on success
 */
s32 vfsr_link(tTid tid,const char *oldPath,const char *newPath);

/**
 * Unlinks the given path. That means, the directory-entry will be removed and if there are no
 * more references to the inode, it will be removed.
 *
 * @param tid the thread-id
 * @param path the path
 * @return 0 on success
 */
s32 vfsr_unlink(tTid tid,const char *path);

/**
 * Writes all dirty objects of the filesystem to disk
 *
 * @param tid the thread-id
 * @return 0 on success
 */
s32 vfsr_sync(tTid tid);

/**
 * Closes the given inode
 *
 * @param tid the thread-id
 * @param file the 'real' file
 * @param inodeNo the inode
 */
void vfsr_closeFile(tTid tid,tFileNo file,tInodeNo inodeNo);

#endif /* VFSREAL_H_ */
