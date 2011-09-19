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

#ifndef FSINTERFACE_H_
#define FSINTERFACE_H_

#include <esc/common.h>
#include <stddef.h>

#define MAX_NAME_LEN		52
#define MAX_PATH_LEN		255

/* file-system-types */
#define FS_TYPE_EXT2		0
#define FS_TYPE_ISO9660		1

/* mode masks */
#define S_IFMT				0170000
#define S_IFSERV			0140000
#define S_IFLNK				0120000
#define S_IFREG				0100000
#define S_IFBLK				0060000
#define S_IFDIR				0040000
#define S_IFCHR				0020000
#define S_IFFS				0010000
#define S_ISUID				0004000
#define S_ISGID				0002000
#define S_ISSTICKY			0001000
#define S_IRWXU				0000700
#define S_IRUSR				0000400
#define S_IWUSR				0000200
#define S_IXUSR				0000100
#define S_IRWXG				0000070
#define S_IRGRP				0000040
#define S_IWGRP				0000020
#define S_IXGRP				0000010
#define S_IRWXO				0000007
#define S_IROTH				0000004
#define S_IWOTH				0000002
#define S_IXOTH				0000001

#define S_ISDIR(mode) 		(((mode) & S_IFMT) == S_IFDIR)
#define S_ISREG(mode) 		(((mode) & S_IFMT) == S_IFREG)
#define S_ISLNK(mode)		(((mode) & S_IFMT) == S_IFLNK)
#define S_ISCHR(mode) 		(((mode) & S_IFMT) == S_IFCHR)
#define S_ISBLK(mode)		(((mode) & S_IFMT) == S_IFBLK)
#define S_ISFS(mode)		(((mode) & S_IFMT) == S_IFFS)
#define S_ISSERV(mode)		(((mode) & S_IFMT) == S_IFSERV)

#define MODE_READ			(S_IRUSR | S_IRGRP | S_IROTH)
#define MODE_WRITE			(S_IWUSR | S_IWGRP | S_IWOTH)
#define MODE_EXEC			(S_IXUSR | S_IXGRP | S_IXOTH)
#define MODE_PERM			(MODE_READ | MODE_WRITE | MODE_EXEC | \
							S_ISUID | S_ISGID | S_ISSTICKY)

/* default modes for file and folder */
#define FILE_DEF_MODE		(S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define DIR_DEF_MODE		(S_IFDIR | \
								S_IRUSR | S_IWUSR | S_IXUSR | \
								S_IRGRP | S_IXGRP | \
								S_IROTH | S_IXOTH)

typedef struct {
	/* ID of device containing file */
	dev_t device;
	inode_t inodeNo;
	/* protection */
	mode_t mode;
	/* number of hard links */
	ushort linkCount;
	/* owner user- and group-id */
	uid_t uid;
	gid_t gid;
	/* total size, in bytes */
	int size;
	/* blocksize for efficent filesystem I/O */
	ushort blockSize;
	/* number of blocks allocated */
	ushort blockCount;
	/* times */
	time_t accesstime;
	time_t modifytime;
	time_t createtime;
} sFileInfo;

/* a directory-entry */
typedef struct {
	inode_t nodeNo;
	uint16_t recLen;
	uint16_t nameLen;
	char name[MAX_NAME_LEN + 1];
} A_PACKED sDirEntry;

#endif /* FSINTERFACE_H_ */
