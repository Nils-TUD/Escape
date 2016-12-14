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

enum {
	DEV_OPEN						= 1 << 0,
	DEV_READ						= 1 << 1,	/* cancelable, if DEV_CANCEL is supported */
	DEV_WRITE						= 1 << 2,	/* cancelable, if DEV_CANCEL is supported */
	DEV_CLOSE						= 1 << 3,
	DEV_SHFILE						= 1 << 4,
	DEV_CANCEL						= 1 << 5,	/* cancel-message */
	DEV_CANCELSIG					= 1 << 6,	/* cancel-signal (SIGCANCEL) */
	DEV_CREATSIBL					= 1 << 7,	/* cancelable, if DEV_CANCEL is supported */
	/* For delegate, keep in mind that a file descriptor can point to all kinds of things. In
	 * particular, it can refer to a device, possibly your own device. Thus, either don't use the
	 * file in the thread that handles the device or, if you need to do that, deny delegations of
	 * devices. Otherwise, you risk a deadlock */
	DEV_DELEGATE					= 1 << 8,	/* accepts file delegations from clients */
	DEV_OBTAIN						= 1 << 9,	/* allows to pass files to clients */
	DEV_SIZE						= 1 << 10,
};

enum {
	/* supports read or write, is byte-oriented and random access is not supported */
	DEV_TYPE_CHAR					= 0,
	/* supports read or write, is a block-oriented device, i.e. random access is supported */
	DEV_TYPE_BLOCK					= 1,
	/* has an arbitrary messaging interface */
	DEV_TYPE_SERVICE				= 2,
	/* has the filesystem interface */
	DEV_TYPE_FS						= 3,
	/* normal file */
	DEV_TYPE_FILE					= 4,
};

static const int BITS_DEV_TYPE		= 3;

static const int GW_NOBLOCK			= 1;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Creates a device at <path>.
 *
 * @param path the path
 * @param mode the mode to set
 * @param type the device-type (DEV_TYPE_*)
 * @param ops the supported operations (DEV_*)
 * @return the file-desc if successfull, < 0 if an error occurred
 */
A_CHECKRET int createdev(const char *path,mode_t mode,uint type,uint ops);

/**
 * Creates a device at <dir>/<name>.
 *
 * @param dir the file descriptor for the directory
 * @param name the device filename
 * @param mode the mode to set
 * @param type the device-type (DEV_TYPE_*)
 * @param ops the supported operations (DEV_*)
 * @return the file-desc if successfull, < 0 if an error occurred
 */
A_CHECKRET int fcreatedev(int dir,const char *name,mode_t mode,uint type,uint ops);

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

/**
 * Binds the device or channel, referenced by <fd>, to the thread with given id.
 * For devices it means that all channels are bound to thread <tid>, i.e. thread <tid> will receive
 * the messages from getwork() from all channels (current and future).
 * For channels it means that this channel gets bound to thread <tid>, i.e. thread <tid> will receive
 * the messages from getwork() from channel <fd>.
 *
 * @param fd the fd for the device or channel
 * @param tid the thread-id
 * @return 0 on success
 */
static inline int bindto(int fd,tid_t tid) {
	return syscall2(SYSCALL_BINDTO,fd,tid);
}

#if defined(__cplusplus)
}
#endif
