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

#ifndef VFSDRV_H_
#define VFSDRV_H_

#include <common.h>
#include <vfs/node.h>

/**
 * Inits the vfs-driver
 */
void vfsdrv_init(void);

/**
 * Sends the open-command to the driver and waits for its response
 *
 * @param tid the thread-id
 * @param file the file for the driver
 * @param node the VFS-node of the driver
 * @param flags the open-flags
 * @return 0 if successfull
 */
s32 vfsdrv_open(tTid tid,tFileNo file,sVFSNode *node,u32 flags);

/**
 * Reads <count> bytes at <offset> into <buffer> from the given driver. Note that not all
 * drivers will use the offset. Some may simply ignore it.
 *
 * @param tid the thread-id
 * @param file the file for the driver
 * @param node the VFS-node of the driver
 * @param buffer the buffer where to copy the data
 * @param offset the offset from which to read
 * @param count the number of bytes to read
 * @return the number of read bytes or a negative error-code
 */
s32 vfsdrv_read(tTid tid,tFileNo file,sVFSNode *node,void *buffer,u32 offset,u32 count);

/**
 * Writes <count> bytes to <offset> from <buffer> to the given driver. Note that not all
 * drivers will use the offset. Some may simply ignore it.
 *
 * @param tid the thread-id
 * @param file the file for the driver
 * @param node the VFS-node of the driver
 * @param buffer the buffer from which copy the data
 * @param offset the offset to write to
 * @param count the number of bytes to write
 * @return the number of written bytes or a negative error-code
 */
s32 vfsdrv_write(tTid tid,tFileNo file,sVFSNode *node,const void *buffer,u32 offset,u32 count);

/**
 * Sends the close-command to the given driver
 *
 * @param tid the thread-id
 * @param file the file for the driver
 * @param node the VFS-node of the driver
 */
void vfsdrv_close(tTid tid,tFileNo file,sVFSNode *node);

#endif /* VFSDRV_H_ */
