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

#ifndef FILE_H_
#define FILE_H_

#include <esc/common.h>
#include "ext2.h"

/**
 * Creates an inode and links it in the given directory with given name
 *
 * @param e the ext2-handle
 * @param dirNode the directory
 * @param name the name
 * @param ino will be set to the inode-number on success
 * @param isDir whether it should be an directory
 * @return 0 on success
 */
s32 ext2_file_create(sExt2 *e,sExt2CInode *dirNode,const char *name,tInodeNo *ino,bool isDir);

/**
 * Deletes the given inode. That means all associated blocks will be free'd
 *
 * @param e the ext2-handle
 * @param cnode the cached inode
 * @return 0 on success
 */
s32 ext2_file_delete(sExt2 *e,sExt2CInode *cnode);

/**
 * Truncates the given file
 *
 * @param e the ext2-handle
 * @param cnode the cached inode
 * @param delete if set the block-numbers in the inode will not be overwritten
 * @return 0 on success
 */
s32 ext2_file_truncate(sExt2 *e,sExt2CInode *cnode,bool delete);

/**
 * Reads <count> bytes at <offset> into <buffer> from the inode with given number. Sets
 * the access-time of the inode!
 *
 * @param e the ext2-handle
 * @param inodeNo the inode-number
 * @param buffer the buffer; if NULL the data will be fetched from disk (if not in cache) but
 * 	not copied anywhere
 * @param offset the offset
 * @param count the number of bytes to read
 * @return the number of read bytes
 */
s32 ext2_file_read(sExt2 *e,tInodeNo inodeNo,void *buffer,u32 offset,u32 count);

/**
 * Reads <count> bytes at <offset> into <buffer> from the given cached inode. It will not
 * set the access-time of it!
 *
 * @param e the ext2-handle
 * @param cnode the cached inode
 * @param buffer the buffer; if NULL the data will be fetched from disk (if not in cache) but
 * 	not copied anywhere
 * @param offset the offset
 * @param count the number of bytes to read
 * @return the number of read bytes
 */
s32 ext2_file_readIno(sExt2 *e,const sExt2CInode *cnode,void *buffer,u32 offset,u32 count);

/**
 * Writes <count> bytes at <offset> from <buffer> to the inode with given number. Will
 * set the modification-time!
 *
 * @param e the ext2-handle
 * @param inodeNo the inode-number
 * @param buffer the buffer
 * @param offset the offset
 * @param count the number of bytes to write
 * @return the number of written bytes
 */
s32 ext2_file_write(sExt2 *e,tInodeNo inodeNo,const void *buffer,u32 offset,u32 count);

/**
 * Writes <count> bytes at <offset> from <buffer> to given cached inode. Will
 * set the modification-time! (-> cnode has to be writable)
 *
 * @param e the ext2-handle
 * @param cnode the cached inode
 * @param buffer the buffer
 * @param offset the offset
 * @param count the number of bytes to write
 * @return the number of written bytes
 */
s32 ext2_file_writeIno(sExt2 *e,sExt2CInode *cnode,const void *buffer,u32 offset,u32 count);

#endif /* FILE_H_ */
