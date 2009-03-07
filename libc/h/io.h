/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef IO_H_
#define IO_H_

#include "common.h"

/* IO flags */
#define IO_READ		1
#define IO_WRITE	2

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
 * Outputs the <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void outb(u16 port,u8 val);

/**
 * Reads the value from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern u8 inb(u16 port);

/**
 * Opens the given path with given mode and returns the associated file-descriptor
 *
 * @param path the path to open
 * @param mode the mode
 * @return the file-descriptor; negative if error
 */
s32 open(cstring path,u8 mode);

/**
 * Reads count bytes from the given file-descriptor into the given buffer and returns the
 * actual read number of bytes.
 *
 * @param fd the file-descriptor
 * @param buffer the buffer to fill
 * @param count the number of bytes
 * @return the actual read number of bytes; negative if an error occurred
 */
s32 read(tFD fd,void *buffer,u32 count);

/**
 * Writes count bytes from the given buffer into the given fd and returns the number of written
 * bytes.
 *
 * @param fd the file-descriptor
 * @param buffer the buffer to read from
 * @param count the number of bytes to write
 * @return the number of bytes written; negative if an error occurred
 */
s32 write(tFD fd,void *buffer,u32 count);

/**
 * Duplicates the given file-descriptor
 *
 * @param fd the file-descriptor
 * @return the error-code or the new file-descriptor
 */
s32 dupFd(tFD fd);

/**
 * Redirects <src> to <dst>. <src> will be closed. Note that both fds have to exist!
 *
 * @param src the source-file-descriptor
 * @param dst the destination-file-descriptor
 * @return the error-code or 0 if successfull
 */
s32 redirFd(tFD src,tFD dst);

/**
 * Closes the given file-descriptor
 *
 * @param fd the file-descriptor
 */
void close(tFD fd);

#endif /* IO_H_ */
