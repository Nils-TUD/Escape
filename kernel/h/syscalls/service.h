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

#ifndef SYSCALLS_SERVICE_H_
#define SYSCALLS_SERVICE_H_

#include <machine/intrpt.h>

/**
 * Registers a service
 *
 * @param const char* name of the service
 * @param u8 type: SINGLE-PIPE OR MULTI-PIPE
 * @return tServ the service-id if successfull
 */
void sysc_regService(sIntrptStackFrame *stack);

/**
 * Unregisters a service
 *
 * @param tServ the service-id
 * @return s32 0 on success or a negative error-code
 */
void sysc_unregService(sIntrptStackFrame *stack);

/**
 * For services: Looks wether a client wants to be served and returns a file-descriptor
 * for it.
 *
 * @param tServ* services an array with service-ids to check
 * @param u32 count the size of <services>
 * @param tServ* serv will be set to the service from which the client has been taken
 * @return tFD the file-descriptor to use
 */
void sysc_getClient(sIntrptStackFrame *stack);

/**
 * For services: Returns the file-descriptor for a specific client
 *
 * @param tServ the service-id
 * @param tTid the thread-id
 * @return tFD the file-descriptor
 */
void sysc_getClientThread(sIntrptStackFrame *stack);

/**
 * For drivers: Sets wether read() has currently something to read or not
 *
 * @param tServ the service-id
 * @param bool wether there is data to read
 * @return s32 0 on success
 */
void sysc_setDataReadable(sIntrptStackFrame *stack);

#endif /* SYSCALLS_SERVICE_H_ */
