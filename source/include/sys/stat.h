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

#define MAX_PATH_LEN		255

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

struct stat {
	dev_t st_dev;     		/* ID of device containing file */
	ino_t st_ino;     		/* inode number */
	mode_t st_mode;    		/* protection */
	nlink_t st_nlink;   	/* number of hard links */
	uid_t st_uid;     		/* user ID of owner */
	gid_t st_gid;     		/* group ID of owner */
	dev_t st_rdev;    		/* device ID (if special file) */
	off_t st_size;    		/* total size, in bytes */
	blksize_t st_blksize; 	/* blocksize for filesystem I/O */
	blkcnt_t st_blocks;  	/* number of 512B blocks allocated */
	time_t st_atime;   		/* time of last access */
	time_t st_mtime;   		/* time of last modification */
	time_t st_ctime;   		/* time of last status change */
};

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Retrieves information about the given file
 *
 * @param path the path of the file
 * @param info will be filled
 * @return 0 on success
 */
A_CHECKRET int stat(const char *path,struct stat *info);

/**
 * Retrieves information about the file behind the given file-descriptor
 *
 * @param fd the file-descriptor
 * @param info will be filled
 * @return 0 on success
 */
A_CHECKRET static inline int fstat(int fd,struct stat *info) {
	return syscall2(SYSCALL_FSTAT,fd,(ulong)info);
}

/**
 * Retrieves only the size of the file referenced by the given file-descriptor. This is only a
 * convenience function since internally, fstat() is used.
 *
 * @param fd the file-descriptor
 * @return the size on success
 */
static inline ssize_t filesize(int fd) {
	struct stat info;
	int res = fstat(fd,&info);
	if(res < 0)
		return res;
	return info.st_size;
}

/**
 * Changes the permissions of the file denoted by <path> to <mode>. This is always possible if
 * the current process is owned by ROOT_UID. Otherwise only if the current process owns the file.
 *
 * @param path the path
 * @param mode the new mode
 * @return 0 on success
 */
A_CHECKRET int chmod(const char *path,mode_t mode);

/**
 * Changes the owner and group of the file denoted by <path> to <uid> and <gid>, respectively. If
 * the current process is owned by ROOT_UID, it can be changed arbitrarily. If the current process
 * owns the file, the group can only be changed to a group this user is a member of. Otherwise
 * the owner can't be changed.
 *
 * @param path the path
 * @param uid the new user-id (-1 = do not change)
 * @param gid the new group-id (-1 = do not change)
 * @return 0 on success
 */
A_CHECKRET int chown(const char *path,uid_t uid,gid_t gid);

/**
 * Checks whether the given path points to a regular file
 *
 * @param path the (absolute!) path
 * @return true if its a file; false if not or an error occurred
 */
bool isfile(const char *path);

/**
 * Checks whether the given path points to a directory
 *
 * @param path the (absolute!) path
 * @return true if its a directory; false if not or an error occurred
 */
bool isdir(const char *path);

/**
 * Checks whether the given path points to a file that behaves like a block-device, i.e. either
 * a regular file or a block device.
 *
 * @param path the (absolute!) path
 * @return true if its a regular file or block device
 */
bool isblock(const char *path);

#if defined(__cplusplus)
}
#endif
