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
s32 requestIOPort(u16 port);

/**
 * Request the given IO-ports
 *
 * @param start the start-port
 * @param count the number of ports to reserve
 * @return a negative error-code or 0 if successfull
 */
s32 requestIOPorts(u16 start,u16 count);

/**
 * Releases the given IO-port
 *
 * @param port the port
 * @return a negative error-code or 0 if successfull
 */
s32 releaseIOPort(u16 port);

/**
 * Releases the given IO-ports
 *
 * @param start the start-port
 * @param count the number of ports to reserve
 * @return a negative error-code or 0 if successfull
 */
s32 releaseIOPorts(u16 start,u16 count);

/**
 * Outputs the byte <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void outByte(u16 port,u8 val);

/**
 * Outputs the word <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void outWord(u16 port,u16 val);

/**
 * Outputs the dword <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void outDWord(u16 port,u32 val);

/**
 * Outputs <count> words at <addr> to port <port>
 *
 * @param port the port
 * @param addr an array of words
 * @param count the number of words to output
 */
void outWordStr(u16 port,const void *addr,u32 count);

/**
 * Reads a byte from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern u8 inByte(u16 port);

/**
 * Reads a word from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern u16 inWord(u16 port);

/**
 * Reads a dword from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern u32 inDWord(u16 port);

/**
 * Reads <count> words to <addr> from port <port>
 *
 * @param port the port
 * @param addr the array to write to
 * @param count the number of words to read
 */
void inWordStr(u16 port,void *addr,u32 count);

#ifdef __cplusplus
}
#endif

#endif /* PORTS_H_ */
