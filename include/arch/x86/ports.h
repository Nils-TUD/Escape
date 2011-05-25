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

#ifndef PORTS_H_
#define PORTS_H_

#include <esc/common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Request the given IO-port
 *
 * @param port the port
 * @return a negative error-code or 0 if successfull
 */
int requestIOPort(uint16_t port) A_CHECKRET;

/**
 * Request the given IO-ports
 *
 * @param start the start-port
 * @param count the number of ports to reserve
 * @return a negative error-code or 0 if successfull
 */
int requestIOPorts(uint16_t start,size_t count) A_CHECKRET;

/**
 * Releases the given IO-port
 *
 * @param port the port
 * @return a negative error-code or 0 if successfull
 */
int releaseIOPort(uint16_t port);

/**
 * Releases the given IO-ports
 *
 * @param start the start-port
 * @param count the number of ports to reserve
 * @return a negative error-code or 0 if successfull
 */
int releaseIOPorts(uint16_t start,size_t count);

/**
 * Outputs the byte <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void outByte(uint16_t port,uint8_t val);

/**
 * Outputs the word <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void outWord(uint16_t port,uint16_t val);

/**
 * Outputs the dword <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void outDWord(uint16_t port,uint32_t val);

/**
 * Outputs <count> words at <addr> to port <port>
 *
 * @param port the port
 * @param addr an array of words
 * @param count the number of words to output
 */
void outWordStr(uint16_t port,const void *addr,size_t count);

/**
 * Reads a byte from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern uint8_t inByte(uint16_t port);

/**
 * Reads a word from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern uint16_t inWord(uint16_t port);

/**
 * Reads a dword from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern uint32_t inDWord(uint16_t port);

/**
 * Reads <count> words to <addr> from port <port>
 *
 * @param port the port
 * @param addr the array to write to
 * @param count the number of words to read
 */
void inWordStr(uint16_t port,void *addr,size_t count);

#ifdef __cplusplus
}
#endif

#endif /* PORTS_H_ */
