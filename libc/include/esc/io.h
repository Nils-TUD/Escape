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

#ifndef IO_H_
#define IO_H_

#include <esc/common.h>
#include <stdarg.h>
#include <fsinterface.h>

/* IO flags */
#define IO_READ					1
#define IO_WRITE				2

/* file descriptors for stdin, stdout and stderr */
#define STDIN_FILENO			0
#define STDOUT_FILENO			1
#define STDERR_FILENO			2

/* seek-types */
#define SEEK_SET				0
#define SEEK_CUR				1
#define SEEK_END				2

/* ioctl-commands */
#define IOCTL_VID_SETCURSOR		0
#define IOCTL_VT_EN_ECHO		1
#define IOCTL_VT_DIS_ECHO		2
#define IOCTL_VT_EN_RDLINE		3
#define IOCTL_VT_DIS_RDLINE		4
#define IOCTL_VT_EN_RDKB		5
#define IOCTL_VT_DIS_RDKB		6
#define IOCTL_VT_EN_NAVI		7
#define IOCTL_VT_DIS_NAVI		8
#define IOCTL_VT_BACKUP			9
#define IOCTL_VT_RESTORE		10

/* ioctl-cursor-pos */
typedef struct {
	u8 col;
	u8 row;
} sIoCtlCursorPos;

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
tFD open(const char *path,u8 mode);

/**
 * Retrieves information about the given file
 *
 * @param path the path of the file
 * @param info will be filled
 * @return 0 on success
 */
s32 getFileInfo(const char *path,sFileInfo *info);

/**
 * Checks wether we are at EOF
 *
 * @param fd the file-descriptor
 * @return 1 if at EOF
 */
s32 eof(tFD fd);

/**
 * Changes the position in the given file. For service-usages SEEK_CUR can be used to skip
 * <offset> messages and SEEK_END to skip all messages.
 *
 * @param fd the file-descriptor
 * @param offset the offset
 * @param whence the seek-type: SEEK_SET, SEEK_CUR or SEEK_END
 * @return the new position on success, of the negative error-code
 */
s32 seek(tFD fd,s32 offset,u32 whence);

/**
 * Reads count bytes from the given file-descriptor into the given buffer and returns the
 * actual read number of bytes.
 *
 * @param fd the file-descriptor
 * @param buffer the buffer to fill
 * @param count the number of bytes
 * @return the actual read number of bytes; negative if an error occurred
 */
s32 read(tFD fd,void *buffer,u32 count);

/**
 * Writes count bytes from the given buffer into the given fd and returns the number of written
 * bytes.
 *
 * @param fd the file-descriptor
 * @param buffer the buffer to read from
 * @param count the number of bytes to write
 * @return the number of bytes written; negative if an error occurred
 */
s32 write(tFD fd,void *buffer,u32 count);

/**
 * Performs the io-control command on the device identified by <fd>. This works with device-
 * drivers only!
 *
 * @param fd the file-descriptor
 * @param cmd the command
 * @param data the data
 * @param size the data-size
 * @return 0 on success
 */
s32 ioctl(tFD fd,u32 cmd,u8 *data,u32 size);

/**
 * Sends a message to the service identified by <fd>.
 *
 * @param fd the file-descriptor
 * @param id the msg-id
 * @param msg the message
 * @param size the size of the message
 * @return 0 on success or < 0 if an error occurred
 */
s32 send(tFD fd,tMsgId id,const void *msg,u32 size);

/**
 * Receives a message from the service identified by <fd>. Blocks if no message is available.
 *
 * @param fd the file-descriptor
 * @param id will be set to the msg-id
 * @param msg the message
 * @param size the (max) size of the message
 * @return the size of the message
 */
s32 receive(tFD fd,tMsgId *id,void *msg,u32 size);

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
 * Closes the given file-descriptor
 *
 * @param fd the file-descriptor
 */
void close(tFD fd);

#ifdef __cplusplus
}
#endif

#endif /* IO_H_ */
