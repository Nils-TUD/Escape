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

#ifndef PORTS_H_
#define PORTS_H_

#include <sys/common.h>

/**
 * Outputs the <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void ports_outByte(uint16_t port,uint8_t val);

/**
 * Outputs the <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void ports_outWord(uint16_t port,uint16_t val);

/**
 * Outputs the <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void ports_outDWord(uint16_t port,uint32_t val);

/**
 * Reads the value from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern uint8_t ports_inByte(uint16_t port);

/**
 * Reads the value from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern uint16_t ports_inWord(uint16_t port);

/**
 * Reads the value from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern uint32_t ports_inDWord(uint16_t port);

#endif /* PORTS_H_ */
