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

#include <sys/common.h>
#include <sys/task/thread.h>

/**
 * Inits the file-descriptors for the initial process <p>
 *
 * @param p the process
 */
void fd_init(Proc *p);

/**
 * Requests the file for the given file-descriptor and increments the usage-count
 *
 * @param fd the file-descriptor
 * @return the file or NULL if the fd is invalid
 */
sFile *fd_request(int fd);

/**
 * Releases the given file, i.e. decrements the usage-count
 *
 * @param file the file
 */
void fd_release(sFile *file);

/**
 * Clones all file-descriptors of the current process to <p>
 *
 * @param p the new process
 */
void fd_clone(Proc *p);

/**
 * Destroyes all file-descriptors of <p>
 *
 * @param p the process to destroy
 */
void fd_destroy(Proc *p);

/**
 * Associates a free file-descriptor with the given file-number
 *
 * @param fileNo the file-number
 * @return the file-descriptor on success
 */
int fd_assoc(sFile *fileNo);

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
 * @return the file that was associated with the fd (or NULL)
 */
sFile *fd_unassoc(int fd);

/**
 * Prints the file-descriptors of <p>
 *
 * @param p the process
 */
void fd_print(Proc *p);
