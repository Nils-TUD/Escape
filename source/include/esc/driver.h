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
#include <esc/messages.h>
#include <stdio.h>

#define DEV_OPEN					1
#define DEV_READ					2
#define DEV_WRITE					4
#define DEV_CLOSE					8
#define DEV_SHFILE					16
#define DEV_CANCEL					32

/* supports read or write, is byte-oriented and random access is not supported */
#define DEV_TYPE_CHAR				0
/* supports read or write, is a block-oriented device, i.e. random access is supported */
#define DEV_TYPE_BLOCK				1
/* has an arbitrary messaging interface */
#define DEV_TYPE_SERVICE			2
/* normal file */
#define DEV_TYPE_FILE				3

#define GW_NOBLOCK					1

typedef void (*fGetData)(FILE *str);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a device at given path.
 *
 * @param path the path
 * @param mode the mode to set
 * @param type the device-type (DEV_TYPE_*)
 * @param ops the supported operations (DEV_*)
 * @return the file-desc if successfull, < 0 if an error occurred
 */
A_CHECKRET static inline int createdev(const char *path,mode_t mode,uint type,uint ops) {
	return syscall4(SYSCALL_CRTDEV,(ulong)path,mode,type,ops);
}

/**
 * For drivers: Looks whether a client wants to be served. If not and GW_NOBLOCK is not provided
 * it waits until a client should be served. if not and GW_NOBLOCK is enabled, it returns an error.
 * If a client wants to be served, the message is fetched from him and a file-descriptor is returned.
 * Note that you may be interrupted by a signal!
 *
 * @param fd the device fd
 * @param mid will be set to the msg-id
 * @param msg the message
 * @param size the (max) size of the message
 * @param flags the flags
 * @return the file-descriptor for the communication with the client
 */
A_CHECKRET static inline int getwork(int fd,msgid_t *mid,void *msg,size_t size,uint flags) {
	return syscall4(SYSCALL_GETWORK,(fd << 2) | flags,(ulong)mid,(ulong)msg,size);
}

#ifdef __cplusplus
}
#endif
