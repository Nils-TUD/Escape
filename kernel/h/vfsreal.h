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
 * Sets the node-number of the FS-service
 *
 * @param nodeNo the service-node
 */
void vfsr_setFSService(tVFSNodeNo nodeNo);

/**
 * Notifies the kernel that there is a message he can read
 */
void vfsr_setGotMsg(void);

/**
 * Checks wether the kernel has received messages from the fs
 */
void vfsr_checkForMsgs(void);

/**
 * Opens the given path with given flags for given process
 *
 * @param pid the process-id
 * @param flags read / write
 * @param path the path
 * @return 0 on success or the error-code
 */
s32 vfsr_openFile(tPid pid,u8 flags,const char *path);

/**
 * Retrieves information about the given (real!) path
 *
 * @param pid the process-id
 * @param path the path in the real filesystem
 * @param info should be filled
 * @return 0 on success
 */
s32 vfsr_getFileInfo(tPid pid,const char *path,sFileInfo *info);

/**
 * Reads from the given inode at <offset> <count> bytes into the given buffer
 *
 * @param pid the process-id
 * @param inodeNo the inode
 * @param buffer the buffer to fill
 * @param offset the offset in the data
 * @param count the number of bytes to copy
 * @return the number of read bytes
 */
s32 vfsr_readFile(tPid pid,tInodeNo inodeNo,u8 *buffer,u32 offset,u32 count);

/**
 * Writes to the given inode at <offset> <count> bytes from the given buffer
 *
 * @param pid the process-id
 * @param inodeNo the inode
 * @param buffer the buffer
 * @param offset the offset in the inode
 * @param count the number of bytes to copy
 * @return the number of written bytes
 */
s32 vfsr_writeFile(tPid pid,tInodeNo inodeNo,u8 *buffer,u32 offset,u32 count);

/**
 * Closes the given inode
 *
 * @param inodeNo the inode
 */
void vfsr_closeFile(tInodeNo inodeNo);

#endif /* VFSREAL_H_ */
