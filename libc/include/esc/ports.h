/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PORTS_H_
#define PORTS_H_

#include <esc/common.h>

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

#endif /* PORTS_H_ */
