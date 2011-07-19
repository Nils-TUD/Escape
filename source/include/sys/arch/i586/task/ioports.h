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

#ifndef I586_IOPORTS_H_
#define I586_IOPORTS_H_

#include <sys/common.h>
#include <sys/task/proc.h>

/**
 * Inits the IO-map for the given process
 *
 * @param p the process
 */
void ioports_init(sProc *p);

/**
 * Requests some IO-ports for the given process. Will not replace the IO-Map in TSS!
 *
 * @param p the process
 * @param start the start-port
 * @param count the number of ports
 * @return the error-code or 0
 */
int ioports_request(sProc *p,uint16_t start,size_t count);

/**
 * Sets the io-map of the given process into the TSS.
 *
 * @param p the process
 */
void ioports_setMap(sProc *p);

/**
 * Handles a GPF for the given process and checks whether the port-map is already set. If not,
 * it will be set.
 *
 * @param p the process
 * @return true if the io-map was not present and is present now
 */
bool ioports_handleGPF(sProc *p);

/**
 * Releases some IO-ports for the given process. Will not replace the IO-Map in TSS!
 *
 * @param p the process
 * @param start the start-port
 * @param count the number of ports
 * @return the error-code or 0
 */
int ioports_release(sProc *p,uint16_t start,size_t count);

/**
 * Free's the io-ports of the given process
 *
 * @param p the process
 */
void ioports_free(sProc *p);


#if DEBUGGING

/**
 * Prints the given IO-map
 *
 * @param map the io-map
 */
void ioports_print(const uint8_t *map);

#endif

#endif /* I586_IOPORTS_H_ */
