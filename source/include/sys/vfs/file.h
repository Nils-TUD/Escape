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

#ifndef FILE_H_
#define FILE_H_

#include <sys/common.h>
#include <sys/vfs/vfs.h>

/**
 * Creates a new file with name <name> in <parent>
 *
 * @param pid the process-id
 * @param parent the parent-node
 * @param name the name
 * @param read the read-function; note that if you use vfs_file_read, it is expected that you
 * 	either specify vfs_file_write, too, or use it.
 * @param write the write-function
 * @return the created node or NULL
 */
sVFSNode *vfs_file_create(pid_t pid,sVFSNode *parent,char *name,fRead read,fWrite write);

/**
 * The default-read-handler
 *
 * @param pid the process-id
 * @param file the file
 * @param node the file-node
 * @param buffer to buffer to write into
 * @param offset the offset
 * @param count the number of bytes to read
 * @return the number of read bytes
 */
ssize_t vfs_file_read(pid_t pid,sFile *file,sVFSNode *node,void *buffer,off_t offset,size_t count);

/**
 * The default write-handler
 *
 * @param pid the process-id
 * @param file the file
 * @param n the file-node
 * @param buffer to buffer to read from
 * @param offset the offset
 * @param count the number of bytes to write
 * @return the number of written bytes
 */
ssize_t vfs_file_write(pid_t pid,sFile *file,sVFSNode *n,const void *buffer,off_t offset,size_t count);

#endif /* FILE_H_ */
