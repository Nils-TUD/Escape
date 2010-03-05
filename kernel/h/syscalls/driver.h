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

#include <machine/intrpt.h>

/**
 * Registers a driver
 *
 * @param const char* name of the driver
 * @param u32 flags: what functions are implemented
 * @return tDrvId the driver-id if successfull
 */
void sysc_regDriver(sIntrptStackFrame *stack);

/**
 * Unregisters a driver
 *
 * @param tDrvId the driver-id
 * @return s32 0 on success or a negative error-code
 */
void sysc_unregDriver(sIntrptStackFrame *stack);

/**
 * For drivers: Returns the file-descriptor for a specific client
 *
 * @param tDrvId the driver-id
 * @param tTid the thread-id
 * @return tFD the file-descriptor
 */
void sysc_getClientThread(sIntrptStackFrame *stack);

/**
 * For drivers: Sets wether read() has currently something to read or not
 *
 * @param tDrvId the driver-id
 * @param bool wether there is data to read
 * @return s32 0 on success
 */
void sysc_setDataReadable(sIntrptStackFrame *stack);

/**
 * For drivers: Looks wether a client wants to be served. If not and WG_NOBLOCK is not provided
 * it waits until a client should be served. if not and WG_NOBLOCK is enabled, it returns an error.
 * If a client wants to be served, the message is fetched from him and the client-id is returned.
 * You can use the client-id for writing a reply.
 *
 * @param tDrvId* an array with driver-ids to check
 * @param u32 the number of driver-ids
 * @param tDrvId* will be set to the driver from which the client has been taken
 * @param tMsgId* will be set to the msg-id
 * @param sMsg* the message
 * @param u32 the (max) size of the message
 * @param u8 flags
 * @return tDrvId the client-id or a negative error-code
 */
void sysc_getWork(sIntrptStackFrame *stack);

/**
 * Sends a reply to the given client.
 *
 * @param tDrvId the client-id
 * @param tMsgId the msg-id
 * @param sMsg* the message
 * @param u32 the size of the message
 * @return s32 0 on success
 */
void sysc_reply(sIntrptStackFrame *stack);

#endif /* SYSCALLS_DRIVER_H_ */
