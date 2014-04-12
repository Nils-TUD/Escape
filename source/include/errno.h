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

#include <esc/common.h>

/* error-codes */
#define EACCES						1	/* Permission denied */
#define EBADF						2	/* Bad file descriptor */
#define EBUSY						3	/* Device or resource busy */
#define ECHILD						4	/* No child processes */
#define EEXIST						5	/* File exists */
#define EFAULT						6	/* Bad address */
#define EINTR						7	/* Interrupted function */
#define EINVAL						8	/* Invalid argument */
#define EISDIR						9	/* Is a directory */
#define EMFILE						10	/* Too many open files */
#define ENOENT						11	/* No such file or directory */
#define ENOSYS						12	/* Function not supported */
#define ENOTDIR						13	/* Not a directory */
#define ENOTEMPTY					14	/* Directory not empty */
#define ENOTSUP						15	/* Not supported */
#define EPERM						16	/* Operation not permitted */
#define EROFS						17	/* Read-only file system */
#define ESPIPE						18	/* Invalid seek */
#define EWOULDBLOCK					19	/* Operation would block */
#define ENOMEM						20	/* Not enough space */
#define ENOBUFS						21	/* No buffer space available */
#define ENOSPC						22	/* No space left on device */
#define EXDEV						23	/* Cross-device link */
#define ENAMETOOLONG				24	/* Filename too long */
#define ENXIO						25	/* No such device or address */
#define ESRCH						26	/* No such process */
#define ENOEXEC						27	/* Execution file format error */
#define ENFILE						28	/* Too many files open in system */
#define ENOCLIENT					29	/* No client waiting to be served */
#define EDESTROYED					30	/* Something existed previously, but has been destroyed */
#define ENOPROCS					31	/* No more process-slots available */
#define ENOTHREADS					32	/* No more thread-slots available */
#define EPIPE						33	/* Broken pipe */
#define ENETUNREACH					34	/* Network unreachable */
#define EAGAIN						35	/* Resource temporarily unavailable */
#define EADDRINUSE					36	/* The given address is in use */
#define ENOTBOUND					37	/* Socket is not bound to an address */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The last error-code
 */
extern int errno;

#ifdef __cplusplus
}
#endif
