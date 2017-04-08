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
#include <errno.h>
#include <stdarg.h>

enum {
	/* access mode */
	O_MSGS					= 1 << 0,	/* exchange messages with a device */
	O_WRITE					= 1 << 1,
	O_READ					= 1 << 2,
	O_EXEC					= O_MSGS,
	O_RDONLY				= O_READ,
	O_WRONLY				= O_WRITE,
	O_RDWR					= O_READ | O_WRITE,
	O_RDWRMSG				= O_READ | O_WRITE | O_MSGS,
	O_ACCMODE	   			= O_RDWRMSG,

	/* open flags */
	O_CREAT					= 1 << 3,
	O_TRUNC					= 1 << 4,
	O_APPEND				= 1 << 5,
	O_NONBLOCK				= 1 << 6,	/* don't block when reading or receiving a msg from devices */
	O_LONELY				= 1 << 7,	/* disallow other accesses */
	O_EXCL					= 1 << 8,	/* fail if the file already exists */
	O_NOCHAN				= 1 << 9, 	/* don't create a channel, but open the device itself */
	O_NOFOLLOW				= 1 << 10,	/* don't resolve last symlink */
};

/* file descriptors for stdin, stdout and stderr */
enum {
	STDIN_FILENO			= 0,
	STDOUT_FILENO			= 1,
	STDERR_FILENO			= 2,
	/* special fd for syscall tracing */
	STRACE_FILENO			= 3,
};

/* fcntl-commands */
enum {
	F_GETFL					= 0,
	F_SETFL					= 1,
	F_GETACCESS				= 2,
	F_SEMUP					= 3,
	F_SEMDOWN				= 4,
};

/* seek-types */
enum {
	SEEK_SET				= 0,
	SEEK_CUR				= 1,
	SEEK_END				= 2,
};

/* reserved delegate arguments */
enum {
	DEL_ARG_SHFILE			= -1,
};

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

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Opens the given path with given mode and returns the associated file-descriptor.
 * If O_CREAT is given in <flags>, the mode should be given as the third argument (see create).
 * Otherwise, the third arguments is ignored.
 *
 * @param path the path to open
 * @param flags the open flags (O_*)
 * @param mode the mode for the created file (only if O_CREAT is set)
 * @return the file-descriptor; negative if error
 */
A_CHECKRET int open(const char *path,uint flags,...);

/**
 * The equivalent of open(path,O_CREAT | O_WRONLY | O_TRUNC,mode).
 *
 * @param path the path to open
 * @param mode the mode for the created file
 * @return the file-descriptor; negative if error
 */
A_CHECKRET static inline int creat(const char *path,mode_t mode) {
	return open(path,O_CREAT | O_WRONLY | O_TRUNC,mode);
}

/**
 * Creates a pipe with 2 separate files for reading and writing.
 *
 * @param readFd will be set to the fd for reading
 * @param writeFd will be set to the fd for writing
 * @return 0 on success
 */
A_CHECKRET int pipe(int *readFd,int *writeFd);

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
 * @param cmd the command (F_*)
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
 * Truncates the file to <length> bytes by either extending it with 0-bytes or cutting it to
 * that length.
 *
 * @param fd the file-descriptor
 * @param length the desired length
 * @return 0 on success
 */
static inline int ftruncate(int fd,off_t length) {
	return syscall2(SYSCALL_TRUNCATE,fd,length);
}

/**
 * Truncates the file to <length> bytes by either extending it with 0-bytes or cutting it to
 * that length.
 *
 * @param path the path to the file
 * @param length the desired length
 * @return 0 on success
 */
int truncate(const char *path,off_t length);

/**
 * Sends a message to the device identified by <fd>.
 *
 * @param fd the file-descriptor
 * @param id the msg-id
 * @param msg the message (may be NULL)
 * @param size the size of the message
 * @return the used message-id on success or < 0 if an error occurred
 */
static inline ssize_t send(int fd,msgid_t id,const void *msg,size_t size) {
	return syscall4(SYSCALL_SEND,fd,id,(ulong)msg,size);
}

/**
 * Receives the message from the device identified by <fd> with id *<id> or the next "for anybody"
 * message if <id> is NULL or *<id> is 0. Blocks if that message is not available.
 * You may be interrupted by a signal (-EINTR)!
 *
 * @param fd the file-descriptor
 * @param id will be set to the msg-id (may be NULL)
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
 * Cancels the message <mid> that is currently in flight on the channel denoted by <fd>. If the
 * device supports it, it waits until it has received the response. This tells us whether the
 * message has been canceled or if the response has already been sent.
 *
 * @param fd the file-descriptor for the channel
 * @param mid the message-id to cancel
 * @return 0 if it has been canceled, 1 if the reply is already available or < 0 on errors
 */
A_CHECKRET static inline int cancel(int fd,msgid_t mid) {
	return syscall2(SYSCALL_CANCEL,fd,mid);
}

/**
 * Delegates access to the file <fd> to the device <dev>. That is, the driver for <dev> receives a
 * file descriptor for the file denoted by <fd>.
 *
 * @param dev the device
 * @param fd the file descriptor to delegate
 * @param perm the permissions to delegate (downgrading only; any flag in O_ACCMODE)
 * @param arg an arbitrary argument that is passed to the driver
 * @return 0 on success
 */
A_CHECKRET static inline int delegate(int dev,int fd,uint perm,int arg) {
	return syscall4(SYSCALL_DELEGATE,dev,fd,perm,arg);
}

/**
 * Obtains access to a file from <dev>.
 *
 * @param dev the device
 * @param arg an arbitrary argument that is passed to the driver
 * @return the file descriptor on success
 */
A_CHECKRET static inline int obtain(int dev,int arg) {
	return syscall2(SYSCALL_OBTAIN,dev,arg);
}

/**
 * Create a shared memory file and mmap's it with size <size>.
 *
 * @param size the size of the buffer
 * @param mem will be set to the buffer address (!= NULL means only the sharing failed)
 * @param flags the flags to pass to mmap (besides MAP_SHARED)
 * @return the file descriptor success
 */
int createbuf(size_t size,void **mem,int flags);

/**
 * Create a shared memory file, mmap's it with size <size> and delegates it to
 * the device denoted by <dev>. If everything worked except for the delegate, *mem will be non-zero
 * but the function returns a negative error-code.
 *
 * @param dev the file descriptor to the device
 * @param size the size of the buffer
 * @param mem will be set to the buffer address (!= NULL means only the sharing failed)
 * @param flags the flags to pass to mmap (besides MAP_SHARED)
 * @return the file descriptor on success
 */
int sharebuf(int dev,size_t size,void **mem,int flags);

/**
 * Destroys the shared memory file, that has been created by sharebuf. That is, it munmaps the given
 * memory region and closes the file.
 *
 * @param mem the memory region
 * @param fd the file descriptor
 */
void destroybuf(void *mem,int fd);

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
 * Redirects <src> to <dst>. <src> will be closed. Note that only <dst> has to exist!
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
A_CHECKRET int link(const char *oldPath,const char *newPath);

/**
 * Creates a hardlink at <dir>/<name> which points to <target>
 *
 * @param target the file descriptor for the target
 * @param dir the file descriptor for the directory of the link
 * @param name the name of the link
 * @return 0 on success
 */
A_CHECKRET int flink(int target,int dir,const char *name);

/**
 * Unlinks the given path. That means, the directory-entry will be removed and if there are no
 * more references to the inode, it will be removed.
 *
 * @param path the path
 * @return 0 on success
 */
A_CHECKRET int unlink(const char *path);

/**
 * Unlinks <dir>/<name>. That means, the directory-entry will be removed and if there are no
 * more references to the inode, it will be removed.
 *
 * @param dir the file descriptor to the directory of the file to unlink
 * @param name the file name to remove
 * @return 0 on success
 */
A_CHECKRET int funlink(int dir,const char *name);

/**
 * Renames <oldPath> to <newPath>. Note that you shouldn't assume that link+unlink works for every
 * filesystem, but use rename instead if you want to rename something. E.g. ftpfs does not have
 * support for link.
 *
 * @param oldPath the path to rename
 * @param newPath the new path
 * @return 0 on success
 */
A_CHECKRET int rename(const char *oldPath,const char *newPath);

/**
 * Renames <olddir>/<oldfile> to <newdir>/<newname>. Note that you shouldn't assume that link+unlink
 * works for every filesystem, but use rename instead if you want to rename something. E.g. ftpfs
 * does not have support for link.
 *
 * @param olddir the file descriptor for the directory of the old file
 * @param oldname the old file name
 * @param newdir the file descriptor for the directory of the new file
 * @param newname the new file name
 * @return 0 on success
 */
A_CHECKRET int frename(int olddir,const char *oldname,int newdir,const char *newname);

/**
 * Creates the given directory. Expects that all except the last path-component exist.
 *
 * @param path the path
 * @param mode the mode for the created file
 * @return 0 on success
 */
A_CHECKRET int mkdir(const char *path,mode_t mode);

/**
 * Creates a directory called <name> in the directory denoted by <fd>.
 *
 * @param fd the file descriptor for the directory to create the new directory in
 * @param name the directory name
 * @param mode the mode for the created directory
 * @return 0 on success
 */
A_CHECKRET int fmkdir(int fd,const char *name,mode_t mode);

/**
 * Removes the given directory. Expects that the directory is empty (except '.' and '..')
 *
 * @param path the path
 * @return 0 on success
 */
A_CHECKRET int rmdir(const char *path);

/**
 * Removes the directory <fd>/<name>.
 *
 * @param fd the file descriptor for the directory to remove the new directory from
 * @param name the directory name
 * @return 0 on success
 */
A_CHECKRET int frmdir(int fd,const char *name);

/**
 * Creates a symlink at <linkpath>, pointing to <target>.
 *
 * @param target the link target
 * @param linkpath the path where to create the link
 * @return 0 on success
 */
A_CHECKRET int symlink(const char *target,const char *linkpath);

/**
 * Creates a symlink at <fd>/<name>, pointing to <target>.
 *
 * @param target the link target
 * @param fd the file descriptor for the directory to create the symlink in
 * @param name the symlink name
 * @return 0 on success
 */
A_CHECKRET int fsymlink(const char *target,int fd,const char *name);

/**
 * Resolves the given path and stores the symlink-free path into <buf>.
 *
 * @param path the path to resolve
 * @param buf the destination
 * @param size the size of <buf>
 * @return the number of bytes placed in <buf> or a negative error code
 */
A_CHECKRET ssize_t readlink(const char *path,char *buf,size_t size);

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
 * @return 0 on success
 */
static inline int close(int fd) {
	return syscall1(SYSCALL_CLOSE,fd);
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

#if defined(__cplusplus)
}
#endif
