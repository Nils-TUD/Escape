/**
 * $Id: io.h 630 2010-04-29 18:41:09Z nasmussen $
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

#ifndef IO_H_
#define IO_H_

#include <esc/common.h>
#include <stdarg.h>
#include <fsinterface.h>

/* IO flags */
#define IO_ACCESSMODE	   		3
#define IO_READ					1
#define IO_WRITE				2
#define IO_CREATE				4
#define IO_TRUNCATE				8
#define IO_APPEND				16

/* file descriptors for stdin, stdout and stderr */
#define STDIN_FILENO			0
#define STDOUT_FILENO			1
#define STDERR_FILENO			2

/* seek-types */
#define SEEK_SET				0
#define SEEK_CUR				1
#define SEEK_END				2

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
tFD open(const char *path,u8 mode) A_CHECKRET;

/**
 * Creates a pipe with 2 separate files for reading and writing.
 *
 * @param readFd will be set to the fd for reading
 * @param writeFd will be set to the fd for writing
 * @return 0 on success
 */
s32 pipe(tFD *readFd,tFD *writeFd) A_CHECKRET;

/**
 * Retrieves information about the given file
 *
 * @param path the path of the file
 * @param info will be filled
 * @return 0 on success
 */
s32 stat(const char *path,sFileInfo *info) A_CHECKRET;

/**
 * Retrieves information about the file behind the given file-descriptor
 *
 * @param fd the file-descriptor
 * @param info will be filled
 * @return 0 on success
 */
s32 fstat(tFD fd,sFileInfo *info) A_CHECKRET;

/**
 * Asks for the current file-position
 *
 * @param fd the file-descriptor
 * @param pos will point to the current file-position on success
 * @return 0 on success
 */
s32 tell(tFD fd,u32 *pos) A_CHECKRET;

/**
 * Checks whether we are at EOF
 *
 * @param fd the file-descriptor
 * @return 1 if at EOF
 */
s32 eof(tFD fd);

/**
 * Checks whether a message is available (for drivers)
 *
 * @param fd the file-descriptor
 * @return 1 if so, 0 if not, < 0 if an error occurred
 */
s32 hasMsg(tFD fd);

/**
 * The  isterm()  function  tests  whether  fd  is an open file descriptor
 * referring to a terminal.
 *
 * @param fd the file-descriptor
 * @return true if it referrs to a terminal
 */
bool isterm(tFD fd);

/**
 * Changes the position in the given file
 *
 * @param fd the file-descriptor
 * @param offset the offset
 * @param whence the seek-type: SEEK_SET, SEEK_CUR or SEEK_END
 * @return the new position on success, of the negative error-code
 */
s32 seek(tFD fd,s32 offset,u32 whence) A_CHECKRET;

/**
 * Reads count bytes from the given file-descriptor into the given buffer and returns the
 * actual read number of bytes.
 *
 * @param fd the file-descriptor
 * @param buffer the buffer to fill
 * @param count the number of bytes
 * @return the actual read number of bytes; negative if an error occurred
 */
s32 read(tFD fd,void *buffer,u32 count) A_CHECKRET;

/**
 * Writes count bytes from the given buffer into the given fd and returns the number of written
 * bytes.
 *
 * @param fd the file-descriptor
 * @param buffer the buffer to read from
 * @param count the number of bytes to write
 * @return the number of bytes written; negative if an error occurred
 */
s32 write(tFD fd,const void *buffer,u32 count) A_CHECKRET;

/**
 * Sends a message to the driver identified by <fd>.
 *
 * @param fd the file-descriptor
 * @param id the msg-id
 * @param msg the message (may be NULL)
 * @param size the size of the message
 * @return 0 on success or < 0 if an error occurred
 */
s32 send(tFD fd,tMsgId id,const void *msg,u32 size);

/**
 * Receives a message from the driver identified by <fd>. Blocks if no message is available.
 *
 * @param fd the file-descriptor
 * @param id will be set to the msg-id (may be NULL to skip the message)
 * @param msg the message (may be NULL to skip the message)
 * @param size the (max) size of the message
 * @return the size of the message
 */
s32 receive(tFD fd,tMsgId *id,void *msg,u32 size) A_CHECKRET;

/**
 * Sends the given data in the data-part of the standard-message. Doesn't wait for the response.
 *
 * @param fd the file-descriptor
 * @param id the msg-id
 * @param data the data to send
 * @param size the size of the data (if too big an error will be reported)
 * @return 0 on success
 */
s32 sendMsgData(tFD fd,tMsgId id,const void *data,u32 size);

/**
 * Sends a message with given id and no data to the given driver, receives a message and
 * copies the data-part of size msg.data.arg1 into <data> with at most <size> bytes.
 *
 * @param fd the file-descriptor
 * @param id the msg-id
 * @param data the buffer where to copy the received data to
 * @param size the size of the buffer
 * @return the number of copied bytes on success
 */
s32 recvMsgData(tFD fd,tMsgId id,void *data,u32 size) A_CHECKRET;

/**
 * Duplicates the given file-descriptor
 *
 * @param fd the file-descriptor
 * @return the error-code or the new file-descriptor
 */
tFD dupFd(tFD fd);

/**
 * Redirects <src> to <dst>. <src> will be closed. Note that both fds have to exist!
 *
 * @param src the source-file-descriptor
 * @param dst the destination-file-descriptor
 * @return the error-code or 0 if successfull
 */
s32 redirFd(tFD src,tFD dst);

/**
 * Creates a hardlink at <newPath> which points to <oldPath>
 *
 * @param oldPath the link-target
 * @param newPath the link-path
 * @return 0 on success
 */
s32 link(const char *oldPath,const char *newPath) A_CHECKRET;

/**
 * Unlinks the given path. That means, the directory-entry will be removed and if there are no
 * more references to the inode, it will be removed.
 *
 * @param path the path
 * @return 0 on success
 */
s32 unlink(const char *path) A_CHECKRET;

/**
 * Creates the given directory. Expects that all except the last path-component exist.
 *
 * @param path the path
 * @return 0 on success
 */
s32 mkdir(const char *path) A_CHECKRET;

/**
 * Removes the given directory. Expects that the directory is empty (except '.' and '..')
 *
 * @param path the path
 * @return 0 on success
 */
s32 rmdir(const char *path) A_CHECKRET;

/**
 * Mounts <device> at <path> with fs <type>
 *
 * @param device the device-path
 * @param path the path to mount at
 * @param type the fs-type
 * @return 0 on success
 */
s32 mount(const char *device,const char *path,u16 type) A_CHECKRET;

/**
 * Unmounts the device mounted at <path>
 *
 * @param path the path
 * @return 0 on success
 */
s32 unmount(const char *path) A_CHECKRET;

/**
 * Writes all dirty objects of the filesystem to disk
 *
 * @return 0 on success
 */
s32 sync(void) A_CHECKRET;

/**
 * Closes the given file-descriptor
 *
 * @param fd the file-descriptor
 */
void close(tFD fd);

#ifdef __cplusplus
}
#endif

#endif /* IO_H_ */
