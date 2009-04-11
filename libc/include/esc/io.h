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
#define IO_READ			1
#define IO_WRITE		2

/* file descriptors for stdin, stdout and stderr */
#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

/**
 * Opens the given path with given mode and returns the associated file-descriptor
 *
 * @param path the path to open
 * @param mode the mode
 * @return the file-descriptor; negative if error
 */
tFD open(const char *path,u8 mode);

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
 * Sets the position for the given file-descriptor. Note that this is not possible
 * for service-usages!
 *
 * @param fd the file-descriptor
 * @param position the new position in the file.
 * @return 0 on success
 */
s32 seek(tFD fd,u32 position);

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

#endif /* IO_H_ */
