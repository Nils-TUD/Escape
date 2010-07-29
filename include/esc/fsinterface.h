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

#ifndef FSINTERFACE_H_
#define FSINTERFACE_H_

#include <esc/common.h>
#include <types.h>
#include <stddef.h>

#define MAX_NAME_LEN		50
#define MAX_PATH_LEN		255

/* file-system-types */
#define FS_TYPE_EXT2		0
#define FS_TYPE_ISO9660		1

/* mode masks */
#define MODE_TYPE_MASK		0170000
#define MODE_TYPE_SOCKET	0140000
#define MODE_TYPE_LINK		0120000
#define MODE_TYPE_FILE		0100000
#define MODE_TYPE_BLOCKDEV	0060000
#define MODE_TYPE_DIR		0040000
#define MODE_TYPE_CHARDEV	0020000
#define MODE_TYPE_FIFO		0010000
#define MODE_SETUID			0004000
#define MODE_SETGID			0002000
#define MODE_STICKY			0001000
#define MODE_OWNER_MASK		0000700
#define MODE_OWNER_READ		0000400
#define MODE_OWNER_WRITE	0000200
#define MODE_OWNER_EXEC		0000100
#define MODE_GROUP_MASK		0000070
#define MODE_GROUP_READ		0000040
#define MODE_GROUP_WRITE	0000020
#define MODE_GROUP_EXEC		0000010
#define MODE_OTHER_MASK		0000007
#define MODE_OTHER_READ		0000004
#define MODE_OTHER_WRITE	0000002
#define MODE_OTHER_EXEC		0000001

#define MODE_IS_DIR(mode) (((mode) & MODE_TYPE_MASK) == MODE_TYPE_DIR)
#define MODE_IS_FILE(mode) (((mode) & MODE_TYPE_MASK) == MODE_TYPE_FILE)
#define MODE_IS_SOCKET(mode) (((mode) & MODE_TYPE_MASK) == MODE_TYPE_SOCKET)
#define MODE_IS_LINK(mode) (((mode) & MODE_TYPE_MASK) == MODE_TYPE_LINK)
#define MODE_IS_CHARDEV(mode) (((mode) & MODE_TYPE_MASK) == MODE_TYPE_CHARDEV)
#define MODE_IS_BLOCKDEV(mode) (((mode) & MODE_TYPE_MASK) == MODE_TYPE_BLOCKDEV)
#define MODE_IS_FIFO(mode) (((mode) & MODE_TYPE_MASK) == MODE_TYPE_FIFO)

typedef struct {
	/* ID of device containing file */
	tDevNo device;
	tInodeNo inodeNo;
	/* protection */
	u16 mode;
	/* number of hard links */
	u16 linkCount;
	/* owner user- and group-id */
	u16 uid;
	u16 gid;
	/* total size, in bytes */
	s32 size;
	/* blocksize for efficent filesystem I/O */
	u16 blockSize;
	/* number of blocks allocated */
	u16 blockCount;
	/* times */
	u32 accesstime;
	u32 modifytime;
	u32 createtime;
} sFileInfo;

/* a directory-entry */
typedef struct {
	tInodeNo nodeNo;
	u16 recLen;
	u16 nameLen;
	char name[MAX_NAME_LEN + 1];
} A_PACKED sDirEntry;

#endif /* FSINTERFACE_H_ */
