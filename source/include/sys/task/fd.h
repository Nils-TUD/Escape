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

#ifndef FD_H_
#define FD_H_

#include <sys/common.h>
#include <sys/task/thread.h>

/**
 * Inits the file-descriptors for the initial process <p>
 *
 * @param p the process
 */
void fd_init(sProc *p);

/**
 * Requests the file for the given file-descriptor and increments the usage-count
 *
 * @param cur the current thread
 * @param fd the file-descriptor
 * @return the file or < 0 if the fd is invalid
 */
file_t fd_request(sThread *cur,int fd);

/**
 * Releases the given file, i.e. decrements the usage-count
 *
 * @param cur the current thread
 * @param file the file
 */
void fd_release(sThread *cur,file_t file);

/**
 * Clones all file-descriptors of the current process to <p>
 *
 * @param t the current thread
 * @param p the new process
 */
void fd_clone(sThread *t,sProc *p);

/**
 * Destroyes all file-descriptors of <p>
 *
 * @param t the current thread
 * @param p the process to destroy
 */
void fd_destroy(sThread *t,sProc *p);

/**
 * Associates a free file-descriptor with the given file-number
 *
 * @param fileNo the file-number
 * @return the file-descriptor on success
 */
int fd_assoc(file_t fileNo);

/**
 * Duplicates the given file-descriptor
 *
 * @param fd the file-descriptor
 * @return the error-code or the new file-descriptor
 */
int fd_dup(int fd);

/**
 * Redirects <src> to <dst>. <src> will be closed. Note that both fds have to exist!
 *
 * @param src the source-file-descriptor
 * @param dst the destination-file-descriptor
 * @return the error-code or 0 if successfull
 */
int fd_redirect(int src,int dst);

/**
 * Releases the given file-descriptor (marks it unused)
 *
 * @param fd the file-descriptor
 * @return the file-number that was associated with the fd (or -EBADF)
 */
file_t fd_unassoc(int fd);

/**
 * Prints the file-descriptors of <p>
 *
 * @param p the process
 */
void fd_print(sProc *p);

#endif /* FD_H_ */
