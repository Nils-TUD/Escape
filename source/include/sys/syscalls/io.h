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

#ifndef SYSCALLS_IO_H_
#define SYSCALLS_IO_H_

#include <sys/intrpt.h>

/**
 * Open-syscall. Opens a given path with given mode and returns the file-descriptor to use
 * to the user.
 *
 * @param const char* the vfs-filename
 * @param uint flags (read / write)
 * @return int the file-descriptor
 */
int sysc_open(sIntrptStackFrame *stack);

/**
 * Manipulates the given file-descriptor, depending on the command
 *
 * @param int the file-descriptor
 * @param uint the command (F_GETFL or F_SETFL)
 * @param int the argument (just used for F_SETFL)
 * @return int >= 0 on success
 */
int sysc_fcntl(sIntrptStackFrame *stack);

/**
 * Creates a pipe with 2 separate files for reading and writing.
 *
 * @param int* will be set to the fd for reading
 * @param int* will be set to the fd for writing
 * @return 0 on success
 */
int sysc_pipe(sIntrptStackFrame *stack);

/**
 * Determines the current file-position
 *
 * @param int file-descriptor
 * @param long* file-position
 * @return int 0 on success
 */
int sysc_tell(sIntrptStackFrame *stack);

/**
 * Tests whether we are at the end of the file (or if there is a message for driver-usages)
 *
 * @param int file-descriptor
 * @return bool true if we are at EOF
 */
int sysc_eof(sIntrptStackFrame *stack);

/**
 * Changes the position in the given file
 *
 * @param int the file-descriptor
 * @param int the offset
 * @param uint the seek-type
 * @return int the new position on success
 */
int sysc_seek(sIntrptStackFrame *stack);

/**
 * Read-syscall. Reads a given number of bytes in a given file at the current position
 *
 * @param int file-descriptor
 * @param void* buffer
 * @param size_t number of bytes
 * @return ssize_t the number of read bytes
 */
int sysc_read(sIntrptStackFrame *stack);

/**
 * Write-syscall. Writes a given number of bytes to a given file at the current position
 *
 * @param int file-descriptor
 * @param void* buffer
 * @param size_t number of bytes
 * @return ssize_t the number of written bytes
 */
int sysc_write(sIntrptStackFrame *stack);

/**
 * Duplicates the given file-descriptor
 *
 * @param int the file-descriptor
 * @return int the error-code or the new file-descriptor
 */
int sysc_dupFd(sIntrptStackFrame *stack);

/**
 * Redirects <src> to <dst>. <src> will be closed. Note that both fds have to exist!
 *
 * @param int src the source-file-descriptor
 * @param int dst the destination-file-descriptor
 * @return int the error-code or 0 if successfull
 */
int sysc_redirFd(sIntrptStackFrame *stack);

/**
 * Closes the given file-descriptor
 *
 * @param int the file-descriptor
 */
int sysc_close(sIntrptStackFrame *stack);

/**
 * Sends a message
 *
 * @param int the file-descriptor
 * @param msgid_t the msg-id
 * @param void* the data
 * @param size_t the size of the data
 * @return ssize_t 0 on success
 */
int sysc_send(sIntrptStackFrame *stack);

/**
 * Receives a message
 *
 * @param int the file-descriptor
 * @param msgid_t the msg-id
 * @param void* the data
 * @return ssize_t the message-size on success
 */
int sysc_receive(sIntrptStackFrame *stack);

/**
 * Retrieves information about the given file
 *
 * @param const char* path the path of the file
 * @param tFileInfo* info will be filled
 * @return int 0 on success
 */
int sysc_stat(sIntrptStackFrame *stack);

/**
 * Retrieves information about the file behind the given file-descriptor
 *
 * @param int the file-descriptor
 * @param tFileInfo* info will be filled
 * @return int 0 on success
 */
int sysc_fstat(sIntrptStackFrame *stack);

/**
 * Writes all dirty objects of the filesystem to disk
 *
 * @return int 0 on success
 */
int sysc_sync(sIntrptStackFrame *stack);

/**
 * Creates a hardlink at <newPath> which points to <oldPath>
 *
 * @param char* the old path
 * @param char* the new path
 * @return int 0 on success
 */
int sysc_link(sIntrptStackFrame *stack);

/**
 * Unlinks the given path. That means, the directory-entry will be removed and if there are no
 * more references to the inode, it will be removed.
 *
 * @param char* path
 * @return int 0 on success
 */
int sysc_unlink(sIntrptStackFrame *stack);

/**
 * Creates the given directory. Expects that all except the last path-component exist.
 *
 * @param char* the path
 * @return ssize_t 0 on success
 */
int sysc_mkdir(sIntrptStackFrame *stack);

/**
 * Removes the given directory. Expects that the directory is empty (except '.' and '..')
 *
 * @param char* the path
 * @return int 0 on success
 */
int sysc_rmdir(sIntrptStackFrame *stack);

/**
 * Mounts <device> at <path> with fs <type>
 *
 * @param char* the device-path
 * @param char* the path to mount at
 * @param uint the fs-type
 * @return int 0 on success
 */
int sysc_mount(sIntrptStackFrame *stack);

/**
 * Unmounts the device mounted at <path>
 *
 * @param char* the path
 * @return int 0 on success
 */
int sysc_unmount(sIntrptStackFrame *stack);

#endif /* SYSCALLS_IO_H_ */

