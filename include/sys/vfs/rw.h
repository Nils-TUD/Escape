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

#ifndef VFSRW_H_
#define VFSRW_H_

#include <sys/common.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>

/* callback function for the default read-handler */
typedef void (*fReadCallBack)(sVFSNode *node,u32 *dataSize,void **buffer);

/**
 * The default-read-handler
 */
s32 vfsrw_readDef(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * Creates space, calls the callback which should fill the space
 * with data and writes the corresponding part to the buffer of the user
 *
 * @param pid the process-id
 * @param node the vfs-node
 * @param buffer the buffer
 * @param offset the offset
 * @param count the number of bytes to copy
 * @param dataSize the total size of the data
 * @param callback the callback-function
 */
s32 vfsrw_readHelper(tPid pid,sVFSNode *node,u8 *buffer,u32 offset,u32 count,u32 dataSize,
		fReadCallBack callback);

/**
 * The read-handler for pipes
 */
s32 vfsrw_readPipe(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * Read-handler for driver-usages (receive a message)
 */
s32 vfsrw_readDrvUse(tPid pid,tFileNo file,sVFSNode *node,tMsgId *id,u8 *data,u32 size);

/**
 * The default write-handler for the VFS
 */
s32 vfsrw_writeDef(tPid pid,tFileNo file,sVFSNode *n,const u8 *buffer,u32 offset,u32 count);

/**
 * The write-handler for pipes
 */
s32 vfsrw_writePipe(tPid pid,tFileNo file,sVFSNode *node,const u8 *buffer,u32 offset,u32 count);

/**
 * Write-handler for driver-usages (send a message)
 */
s32 vfsrw_writeDrvUse(tPid pid,tFileNo file,sVFSNode *n,tMsgId id,const u8 *data,u32 size);


#if DEBUGGING

/**
 * Prints the given message
 *
 * @param m the message
 */
void vfsrw_dbg_printMessage(void *m);

#endif

#endif /* VFSRW_H_ */
