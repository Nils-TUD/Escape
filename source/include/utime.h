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

struct utimbuf {
	time_t actime;		/* access time */
	time_t modtime;		/* modification time */
};

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Changes the last access and modification time of the file denoted by <path>.
 *
 * @param path the file path
 * @param times the new access and modification times (NULL sets it to the current time)
 * @return 0 on success
 */
int utime(const char *path,const struct utimbuf *times);

/**
 * Changes the last access and modification time of the given file.
 *
 * @param fd the file descriptor.
 * @param times the new access and modification times (NULL sets it to the current time)
 * @return 0 on success
 */
static inline int futime(int fd,const struct utimbuf *times) {
	return syscall2(SYSCALL_UTIME,fd,(ulong)times);
}

#if defined(__cplusplus)
}
#endif
