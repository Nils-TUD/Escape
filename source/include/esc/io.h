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

#pragma once

#include <esc/common.h>
#include <esc/fsinterface.h>
#include <esc/syscalls.h>
#include <stdarg.h>
#include <errno.h>

/* IO flags */
#define IO_ACCESSMODE	   		3
#define IO_READ					1
#define IO_WRITE				2
#define IO_CREATE				4
#define IO_TRUNCATE				8
#define IO_APPEND				16
#define IO_NOBLOCK				32	/* don't block when reading or receiving a msg from devices */
#define IO_MSGS					64	/* exchange messages with a device */
#define IO_SEM					64	/* use semaphore operations */
#define IO_EXCLUSIVE			128	/* disallow other accesses */

/* file descriptors for stdin, stdout and stderr */
#define STDIN_FILENO			0
#define STDOUT_FILENO			1
#define STDERR_FILENO			2

/* fcntl-commands */
#define F_GETFL					0
#define F_SETFL					1
#define F_SETDATA				2
#define F_GETACCESS				3
#define F_SETUNUSED				4
#define F_SEMUP					5
#define F_SEMDOWN				6

/* seek-types */
#define SEEK_SET				0
#define SEEK_CUR				1
#define SEEK_END				2

/* retry a syscall until it succeeded, skipping tries that failed because of a signal */
#define IGNSIGS(expr) ({ \
		int __err; \
		do { \
			__err = (expr); \
		} \
		while(__err == -EINTR); \
		__err; \
	})

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opens the given path with given mode and returns the associated file-descriptor
 *
 * @param path the path to open
 * @param mode the mode
 * @return the file-descriptor; negative if error
 */
A_CHECKRET static inline int open(const char *path,uint mode) {
	return syscall2(SYSCALL_OPEN,(ulong)path,mode);
}

/**
 * Creates a pipe with 2 separate files for reading and writing.
 *
 * @param readFd will be set to the fd for reading
 * @param writeFd will be set to the fd for writing
 * @return 0 on success
 */
A_CHECKRET static inline int pipe(int *readFd,int *writeFd) {
	return syscall2(SYSCALL_PIPE,(ulong)readFd,(ulong)writeFd);
}

/**
 * Retrieves information about the given file
 *
 * @param path the path of the file
 * @param info will be filled
 * @return 0 on success
 */
A_CHECKRET static inline int stat(const char *path,sFileInfo *info) {
	return syscall2(SYSCALL_STAT,(ulong)path,(ulong)info);
}

/**
 * Retrieves information about the file behind the given file-descriptor
 *
 * @param fd the file-descriptor
 * @param info will be filled
 * @return 0 on success
 */
A_CHECKRET static inline int fstat(int fd,sFileInfo *info) {
	return syscall2(SYSCALL_FSTAT,fd,(ulong)info);
}

/**
 * Changes the permissions of the file denoted by <path> to <mode>. This is always possible if
 * the current process is owned by ROOT_UID. Otherwise only if the current process owns the file.
 *
 * @param path the path
 * @param mode the new mode
 * @return 0 on success
 */
A_CHECKRET static inline int chmod(const char *path,mode_t mode) {
	return syscall2(SYSCALL_CHMOD,(ulong)path,mode);
}

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
A_CHECKRET static inline int chown(const char *path,uid_t uid,gid_t gid) {
	return syscall3(SYSCALL_CHOWN,(ulong)path,uid,gid);
}

/**
 * Asks for the current file-position
 *
 * @param fd the file-descriptor
 * @param pos will point to the current file-position on success
 * @return 0 on success
 */
A_CHECKRET static inline int tell(int fd,off_t *pos) {
	return syscall2(SYSCALL_TELL,fd,(ulong)pos);
}

/**
 * Manipulates the given file-descriptor, depending on the command
 *
 * @param fd the file-descriptor
 * @param cmd the command (F_GETFL or F_SETFL)
 * @param arg the argument (just used for F_SETFL)
 * @return >= 0 on success
 */
static inline int fcntl(int fd,uint cmd,int arg) {
	return syscall3(SYSCALL_FCNTL,fd,cmd,arg);
}

/**
 * Changes the position in the given file
 *
 * @param fd the file-descriptor
 * @param offset the offset
 * @param whence the seek-type: SEEK_SET, SEEK_CUR or SEEK_END
 * @return the new position on success, of the negative error-code
 */
A_CHECKRET static inline off_t seek(int fd,off_t offset,uint whence) {
	return syscall3(SYSCALL_SEEK,fd,offset,whence);
}

/**
 * Reads count bytes from the given file-descriptor into the given buffer and returns the
 * actual read number of bytes. You may be interrupted by a signal (-EINTR)!
 *
 * @param fd the file-descriptor
 * @param buffer the buffer to fill
 * @param count the number of bytes
 * @return the actual read number of bytes; negative if an error occurred
 */
A_CHECKRET static inline ssize_t read(int fd,void *buffer,size_t count) {
	return syscall3(SYSCALL_READ,fd,(ulong)buffer,count);
}

/**
 * Writes count bytes from the given buffer into the given fd and returns the number of written
 * bytes.
 *
 * @param fd the file-descriptor
 * @param buffer the buffer to read from
 * @param count the number of bytes to write
 * @return the number of bytes written; negative if an error occurred
 */
A_CHECKRET static inline ssize_t write(int fd,const void *buffer,size_t count) {
	return syscall3(SYSCALL_WRITE,fd,(ulong)buffer,count);
}

/**
 * Sends a message to the device identified by <fd>.
 *
 * @param fd the file-descriptor
 * @param id the msg-id
 * @param msg the message (may be NULL)
 * @param size the size of the message
 * @return 0 on success or < 0 if an error occurred
 */
static inline ssize_t send(int fd,msgid_t id,const void *msg,size_t size) {
	return syscall4(SYSCALL_SEND,fd,id,(ulong)msg,size);
}

/**
 * Receives a message from the device identified by <fd>. Blocks if no message is available.
 * You may be interrupted by a signal (-EINTR)!
 *
 * @param fd the file-descriptor
 * @param id will be set to the msg-id (may be NULL to skip the message)
 * @param msg the message (may be NULL to skip the message)
 * @param size the (max) size of the message
 * @return the size of the message
 */
A_CHECKRET static inline ssize_t receive(int fd,msgid_t *id,void *msg,size_t size) {
	return syscall4(SYSCALL_RECEIVE,fd,(ulong)id,(ulong)msg,size);
}

/**
 * Duplicates the given file-descriptor
 *
 * @param fd the file-descriptor
 * @return the error-code or the new file-descriptor
 */
static inline int dup(int fd) {
	return syscall1(SYSCALL_DUPFD,fd);
}

/**
 * Redirects <src> to <dst>. <src> will be closed. Note that both fds have to exist!
 *
 * @param src the source-file-descriptor
 * @param dst the destination-file-descriptor
 * @return the error-code or 0 if successfull
 */
static inline int redirect(int src,int dst) {
	return syscall2(SYSCALL_REDIRFD,src,dst);
}

/**
 * Creates a hardlink at <newPath> which points to <oldPath>
 *
 * @param oldPath the link-target
 * @param newPath the link-path
 * @return 0 on success
 */
A_CHECKRET static inline int link(const char *oldPath,const char *newPath) {
	return syscall2(SYSCALL_LINK,(ulong)oldPath,(ulong)newPath);
}

/**
 * Unlinks the given path. That means, the directory-entry will be removed and if there are no
 * more references to the inode, it will be removed.
 *
 * @param path the path
 * @return 0 on success
 */
A_CHECKRET static inline int unlink(const char *path) {
	return syscall1(SYSCALL_UNLINK,(ulong)path);
}

/**
 * Creates the given directory. Expects that all except the last path-component exist.
 *
 * @param path the path
 * @return 0 on success
 */
A_CHECKRET static inline int mkdir(const char *path) {
	return syscall1(SYSCALL_MKDIR,(ulong)path);
}

/**
 * Removes the given directory. Expects that the directory is empty (except '.' and '..')
 *
 * @param path the path
 * @return 0 on success
 */
A_CHECKRET static inline int rmdir(const char *path) {
	return syscall1(SYSCALL_RMDIR,(ulong)path);
}

/**
 * Mounts <device> at <path> with fs <type>
 *
 * @param device the device-path
 * @param path the path to mount at
 * @param type the fs-type
 * @return 0 on success
 */
A_CHECKRET static inline int mount(const char *device,const char *path,uint type) {
	return syscall3(SYSCALL_MOUNT,(ulong)device,(ulong)path,type);
}

/**
 * Unmounts the device mounted at <path>
 *
 * @param path the path
 * @return 0 on success
 */
A_CHECKRET static inline int unmount(const char *path) {
	return syscall1(SYSCALL_UNMOUNT,(ulong)path);
}

/**
 * Writes all dirty objects of the filesystem to disk
 *
 * @return 0 on success
 */
A_CHECKRET static inline int sync(void) {
	return syscall0(SYSCALL_SYNC);
}

/**
 * Closes the given file-descriptor
 *
 * @param fd the file-descriptor
 */
static inline void close(int fd) {
	syscall1(SYSCALL_CLOSE,fd);
}

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

#ifdef __cplusplus
}
#endif
