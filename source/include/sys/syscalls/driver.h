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

#ifndef SYSCALLS_DRIVER_H_
#define SYSCALLS_DRIVER_H_

#include <sys/intrpt.h>

/**
 * Registers a driver
 *
 * @param const char* name of the driver
 * @param uint flags: what functions are implemented
 * @return tFD the file-desc if successfull
 */
int sysc_regDriver(sIntrptStackFrame *stack);

/**
 * Fetches the client-id from the given file-descriptor
 *
 * @param tFD the file-descriptor
 * @return tInodeNo the id or an error
 */
int sysc_getClientId(sIntrptStackFrame *stack);

/**
 * For drivers: Returns the file-descriptor for a specific client
 *
 * @param tFD the file-descriptor for the driver
 * @param tInodeNo the client-id
 * @return tFD the file-descriptor
 */
int sysc_getClient(sIntrptStackFrame *stack);

/**
 * For drivers: Looks whether a client wants to be served. If not and WG_NOBLOCK is not provided
 * it waits until a client should be served. if not and WG_NOBLOCK is enabled, it returns an error.
 * If a client wants to be served, the message is fetched from him and the client-id is returned.
 * You can use the client-id for writing a reply.
 *
 * @param tFD* an array of file-descriptors to check
 * @param size_t the number of fds
 * @param tFD* will be set to the file-desc from which the client has been taken
 * @param tMsgId* will be set to the msg-id
 * @param void* the message
 * @param size_t the (max) size of the message
 * @param uint flags
 * @return tFD the file-desc to serve the client or a negative error-code
 */
int sysc_getWork(sIntrptStackFrame *stack);

#endif /* SYSCALLS_DRIVER_H_ */
