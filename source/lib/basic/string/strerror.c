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

#include <stddef.h>
#include <errno.h>
#include <string.h>

static const char *msgs[] = {
	/* 0 (No error) */			"No error",
	/* 1 (EACCES) */			"Permission denied",
	/* 2 (EBADF) */				"Bad file descriptor",
	/* 3 (EBUSY) */				"Device or resource busy",
	/* 4 (ECHILD) */			"No child processes",
	/* 5 (EEXIST) */			"File exists",
	/* 6 (EFAULT) */			"Bad address",
	/* 7 (EINTR) */				"Interrupted function",
	/* 8 (EINVAL) */			"Invalid argument",
	/* 9 (EISDIR) */			"Is a directory",
	/* 10 (EMFILE) */			"Too many open files",
	/* 11 (ENOENT) */			"No such file or directory",
	/* 12 (ENOSYS) */			"Function not supported",
	/* 13 (ENOTDIR) */			"Not a directory",
	/* 14 (ENOTEMPTY) */		"Directory not empty",
	/* 15 (ENOTSUP) */			"Not supported",
	/* 16 (EPERM) */			"Operation not permitted",
	/* 17 (EROFS) */			"Read-only file system",
	/* 18 (ESPIPE) */			"Invalid seek",
	/* 19 (EWOULDBLOCK) */		"Operation would block",
	/* 20 (ENOMEM) */			"Not enough space",
	/* 21 (ENOBUFS) */			"No buffer space available",
	/* 22 (ENOSPC) */			"No space left on device",
	/* 23 (EXDEV) */			"Cross-device link",
	/* 24 (ENAMETOOLONG) */		"Filename too long",
	/* 25 (ENXIO) */			"No such device or address",
	/* 26 (ESRCH) */			"No such process",
	/* 27 (ENOEXEC) */			"Execution file format error",
	/* 28 (ENFILE) */			"Too many files open in system",
	/* 29 (ENOCLIENT) */		"No client waiting to be served",
	/* 30 (EDESTROYED) */		"Something existed previously, but has been destroyed",
	/* 31 (ENOPROCS) */			"No more process-slots available",
	/* 32 (ENOTHREADS) */		"No more thread-slots available",
	/* 33 (EPIPE) */			"Broken pipe",
	/* 34 (ENETUNREACH) */		"Network unreachable",
};

char *strerror(int errnum) {
	if((size_t)errnum < ARRAY_SIZE(msgs))
		return (char*)msgs[errnum];
	else
		return (char*)msgs[0];
}
