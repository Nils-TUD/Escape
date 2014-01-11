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
#define F_DISMSGS				7

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

/* do a sendrecv() and skip tries that failed because of a signal */
#define SENDRECV_IGNSIGS(fd,mid,msg,size) ({ \
	int __err = sendrecv(fd,mid,msg,size); \
	while(__err == -EINTR) { \
		__err = receive(fd,mid,msg,size); \
	} \
	__err; \
})

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opens the given path with given mode and returns the associated file-descriptor
 *
 * @param path the path to open
 * @param flags the open flags (IO_*)
 * @return the file-descriptor; negative if error
 */
A_CHECKRET static inline int open(const char *path,uint flags) {
	return syscall3(SYSCALL_OPEN,(ulong)path,flags,FILE_DEF_MODE);
}

/**
 * Creates the given path with given flags and mode and returns the associated file-descriptor
 *
 * @param path the path to open
 * @param flags the open flags (IO_*)
 * @param mode the mode for the created file
 * @return the file-descriptor; negative if error
 */
A_CHECKRET static inline int create(const char *path,uint flags,mode_t mode) {
	return syscall3(SYSCALL_OPEN,(ulong)path,flags | IO_CREATE,mode);
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
 * Sends a message to the device identified by <fd> and receives the response.
 * Note that if you get interrupted by a signal, you'll receive -EINTR, but the message will
 * have been already sent in all cases, because it can only happen during the receive.
 *
 * @param fd the file-descriptor
 * @param id the msg-id; will hold the received message-id afterwards
 * @param msg the message (may be NULL)
 * @param size the size of the message
 * @return 0 on success or < 0 if an error occurred
 */
A_CHECKRET static inline ssize_t sendrecv(int fd,msgid_t *id,void *msg,size_t size) {
	return syscall4(SYSCALL_SENDRECV,fd,(ulong)id,(ulong)msg,size);
}

/**
 * Shares the file, denoted by <mem>, with the device, denoted by <dev>. That is, it sends
 * a message to the device and asks him to join that memory, if he wants to. The parameter <mem>
 * has to point to the beginning of the area where a file has been mmap'd.
 * If a read or write is performed using <mem> as buffer, the data will be exchanged over this
 * shared memory instead of copying it twice.
 *
 * @param dev the file descriptor to the device
 * @param fd the file descriptor to the shared-memory-file
 * @param mem the memory area where <fd> has been mmap'd
 * @return 0 on success
 */
A_CHECKRET static inline int sharefile(int dev,void *mem) {
	return syscall2(SYSCALL_SHAREFILE,dev,(ulong)mem);
}

/**
 * Uses pshm_create() to create a shared memory file, mmap's it with size <size> and shares it with
 * the device denoted by <dev>. If everything worked except for the sharing, *mem will be non-zero
 * but the function returns a negative error-code.
 *
 * @param dev the file descriptor to the device
 * @param size the size of the buffer
 * @param mem will be set to the buffer address (!= NULL means only the sharing failed)
 * @param name will be set to the shared memory name
 * @param flags the flags to pass to mmap (besides MAP_SHARED)
 * @return 0 on success
 */
int sharebuf(int dev,size_t size,void **mem,ulong *name,int flags);

/**
 * Joins the given shared memory file, i.e. opens <path> and mmaps it.
 *
 * @param path the sh-mem file
 * @param size the size to pass to mmap
 * @param flags the flags to pass to mmap (besides MAP_SHARED)
 * @return the address or NULL
 */
void *joinbuf(const char *path,size_t size,int flags);

/**
 * Destroys the shared memory file, that has been created by sharebuf. That is, it unlinks the file
 * and munmaps the given memory region
 *
 * @param mem the memory region
 * @param name the name of the shared memory file
 */
void destroybuf(void *mem,ulong name);

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
 * Mounts the device denoted by <fd> at <path>
 *
 * @param fd the file-descriptor to the filesystem that should be mounted
 * @param path the path to mount at
 * @return 0 on success
 */
A_CHECKRET static inline int mount(int fd,const char *path) {
	return syscall2(SYSCALL_MOUNT,fd,(ulong)path);
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
 * @return the id of the mountspace of the current process
 */
static inline int getmsid(void) {
	return syscall0(SYSCALL_GETMSID);
}

/**
 * Clones the mountspace of the current process. That is, it creates a new mountspace for which
 * mount and unmount operations don't affect others.
 *
 * @return the new mountspace id on success
 */
static inline int clonems(void) {
	return syscall0(SYSCALL_CLONEMS);
}

/**
 * Joins the mountspace denoted by <id>.
 *
 * @param id the mountspace to join
 * @return 0 on success
 */
static inline int joinms(int id) {
	return syscall1(SYSCALL_JOINMS,id);
}

/**
 * Writes all dirty objects of the affected filesystem to disk
 *
 * @param fd the file-descriptor to some file on that fs
 * @return 0 on success
 */
A_CHECKRET static inline int syncfs(int fd) {
	return syscall1(SYSCALL_SYNCFS,fd);
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
 * Performs the semaphore up-operation on the given file.
 *
 * @param fd the file-descriptor
 * @return 0 on success
 */
static inline int fsemup(int fd) {
	return fcntl(fd,F_SEMUP,0);
}

/**
 * Performs the semaphore down-operation on the given file.
 *
 * @param fd the file-descriptor
 * @return 0 on success
 */
static inline int fsemdown(int fd) {
	return fcntl(fd,F_SEMDOWN,0);
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
