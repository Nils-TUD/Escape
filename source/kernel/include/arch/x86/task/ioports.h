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

#include <task/proc.h>
#include <common.h>

class IOPorts {
public:
	/**
	 * Inits the IO-map for the given process
	 *
	 * @param p the process
	 */
	static void init(Proc *p);

	/**
	 * Requests some IO-ports for the current process. Will not replace the IO-Map in TSS!
	 *
	 * @param start the start-port
	 * @param count the number of ports
	 * @return the error-code or 0
	 */
	static int request(uint16_t start,size_t count);

	/**
	 * Handles a GPF for the current process and checks whether the port-map is already set. If not,
	 * it will be set.
	 *
	 * @return true if the io-map was not present and is present now
	 */
	static bool handleGPF();

	/**
	 * Releases some IO-ports for the current process. Will not replace the IO-Map in TSS!
	 *
	 * @param start the start-port
	 * @param count the number of ports
	 * @return the error-code or 0
	 */
	static int release(uint16_t start,size_t count);

	/**
	 * Free's the io-ports of the given process
	 *
	 * @param p the process
	 */
	static void free(Proc *p);

	/**
	 * Prints the given IO-map
	 *
	 * @param os the output-stream
	 * @param map the io-map
	 */
	static void print(OStream &os,const uint8_t *map);
};
